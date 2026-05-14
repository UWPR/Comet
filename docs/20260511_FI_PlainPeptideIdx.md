# Building a Unique Peptide → Protein Index from a FASTA 

Current Comet calls the RunSearch() function which parses a FASTA file, generating peptides
within the specified enzyme and mass constraints, and stores the peptides in g_pvDBIndex.
After the FASTA has been parsed, these peptides are made unique while storing all protein
references to each unique peptide.

For a human protein FASTA with no enyzme constraint and peptides of length 8 to 25, the
size of the peptides is enormous.  There are on the order of 20 to 100 million amino
acids (depending on canonical or isoforms).  And the number of substrings of length
of length 8 to 25 is on the order of nearly 900 million peptides. 

Let's generate the unique set of peptides including tracking each peptide's protein list
in a more efficient manner.  So when the plain peptide .idx file needs to be generated
for a fragment ion index search, it will call a new function (instead of RunSearch)
to generate that plain peptide file more efficiently.

The real problem is:

# Build an inverted peptide index

You want:

```text
peptide -> list of proteins containing it
```

This is fundamentally similar to:
- search engine indexing
- k-mer indexing in genomics
- suffix-array/suffix-tree problems

and it changes the optimal strategy quite a bit.

---

# Key Insight

You absolutely should track proteins DURING peptide generation.

Do NOT:
1. Generate all peptides
2. Deduplicate
3. Re-scan proteins later

That would roughly double your runtime and massively increase I/O.

Instead:

# Generate peptide → immediately append protein ID

Conceptually:

```text
index[peptide].append(protein_id)
```

The challenge is doing this without exploding RAM.

---

# Important Scale Observation

For human proteins:

Lengths 8–25 produce:

- hundreds of millions to billions of peptide instances
- but far fewer UNIQUE peptides

The number of unique peptides depends strongly on:
- peptide length
- isoforms included
- whether I/L are merged; this will be controlled by the parameter g_staticParams.options.bTreatSameIL
- modifications

Typical rough scales:

| Length | Unique Count |
|---|---|
| 8 | many duplicates |
| 9 | fewer duplicates |
| 12+ | mostly unique |

By 15–25 aa, most peptides occur once.

This matters because:
- short peptides dominate protein multiplicity
- long peptides dominate storage

---

# Best Data Structure

# Hash Table:
# peptide → protein set

But peptide representation matters enormously.

---

# DO NOT Use Strings

Instead use packed encodings.

---

# Best Peptide Representation

## 5-bit amino acid encoding

20 AA fit in 5 bits.

Thus:

| Length | Bits |
|---|---|
| 8 | 40 |
| 9 | 45 |
| 12 | 60 |
| 25 | 125 |

This is excellent.

---

# Huge Optimization:
# Separate Strategies by Length

This is the real clever trick.

---

# Length ≤ 12:
# Fully Packed Integer Representation

For peptides up to 12 AA:

```text
12 * 5 = 60 bits
```

Meaning:
- peptide fits entirely in uint64
- hashing becomes trivial
- comparisons become trivial

Then:

```c
unordered_map<uint64, vector<uint32>>
```

becomes extremely efficient.

This is likely the fastest approach possible.

---

# Length > 12:
# Use Hashes or Compact Encodings

For longer peptides:
- 128-bit packed encoding
- rolling hashes
- or string interning

Long peptides are mostly unique anyway.

So you can often:
- store them compressed
- or spill directly to disk

---

# CRITICAL Optimization:
# Avoid Duplicate Protein Entries

Suppose a peptide appears multiple times within the same protein.

You probably want:

```text
peptide -> unique protein IDs
```

NOT:

```text
peptide -> all occurrences
```

Then avoid repeated insertions.

---

# Efficient Within-Protein Dedup

For each protein:

Maintain temporary local set:

```text
seen_in_this_protein
```

Then:

```python
if peptide not in seen_in_this_protein:
    global_index[peptide].append(protein_id)
```

This avoids huge redundancy.

Very important for repetitive proteins.

---

# Best Overall Architecture

# Two-Level Deduplication

## Level 1
Within-protein dedup

## Level 2
Global peptide index

This minimizes:
- memory
- append operations
- vector growth
- hash collisions

---

# EVEN BETTER:
# Sort-Based Construction

For truly massive scale, hash tables eventually become memory-limited.

Then the optimal approach becomes:

# External sort + reduce

This is exactly MapReduce-style indexing.

---

# Streaming Pipeline

Generate tuples:

```text
(peptide_encoding, protein_id)
```

Write sequentially.

Then:

## External sort by peptide

Result:

```text
AAAAAAAQ -> P1
AAAAAAAQ -> P7
AAAAAAAQ -> P9
AAAAAAAR -> P2
```

Then single linear reduction pass:

```text
AAAAAAAQ -> [P1,P7,P9]
```

This is often better than giant hash tables.

---

# Why Sort-Based Is So Powerful

Advantages:

## 1. Sequential I/O only
Very cache-friendly.

## 2. Bounded RAM
Works at arbitrary scale.

## 3. Easy parallelization

## 4. Natural deduplication

## 5. Protein aggregation becomes trivial

---

# This Is Probably The Best Architecture

For human-scale proteomics:

I would likely choose:

# Phase 1: Generate

Emit:

```text
(peptide_key, protein_id)
```

Where:
- peptide_key = packed peptide or hash

---

# Phase 2: External Sort

Sort by:
1. peptide_key
2. protein_id

---

# Phase 3: Reduce

Single pass:

```text
current peptide:
    collect unique proteins
    emit peptide -> protein list
```

This is extremely scalable.

---

# Another Important Question:
# Exact Sequence vs Hash

You must decide:

## Option A: Exact packed sequence

Pros:
- collision-free
- reconstructable

Cons:
- slightly larger

---

## Option B: 64-bit rolling hash

Pros:
- very compact
- fast

Cons:
- possible collisions

For proteomics:
- collisions may be unacceptable

Thus:
- exact encoding is preferred

---

# Clever Hybrid Strategy

This is probably near-optimal.

---

# Lengths 8–12

Use:
- exact uint64 packed peptide

In-memory hash indexing.

Very fast.

---

# Lengths 13–25

Since most are unique:
- use sort-based streaming
- maybe disk-backed

because hash-table overhead dominates.

---

# Another Major Optimization

# Protein Sets Compress Extremely Well

Most peptides map to:
- 1 protein
- few proteins

Use:

```text
small-vector optimization
```

or:

```text
delta-encoded sorted protein IDs
```

Compression becomes excellent.

---

# Ultimate High-End Solution

If you want truly industrial-scale performance:

Look into:
- finite-state transducers (FSTs)
- minimal perfect hashing
- succinct tries
- suffix arrays
- FM-indexes

But these are probably unnecessary unless:
- you are building a search engine
- handling PTMs
- indexing billions of variants

---

# My Concrete Recommendation

# Best Practical Solution

## Generate:
```text
(peptide_encoding, protein_id)
```

during peptide creation.

## Deduplicate within protein immediately.

## Then external sort.

## Then reduce into:
```text
peptide -> unique protein list
```

This is likely the cleanest and most scalable architecture.

---

# Final Important Insight

Once you require:

```text
peptide -> proteins
```

the problem is no longer primarily:
- peptide generation

It becomes:
- large-scale indexing

So algorithms from:
- databases
- IR/search engines
- genomics k-mer indexing

become more relevant than classical proteomics digestion algorithms.
