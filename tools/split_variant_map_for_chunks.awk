#!/usr/bin/awk -f
# split_variant_map_for_chunks.awk
#
# Splits an idx_to_carafe.py variant-map sidecar (row_index -> iWhichPeptide/modNumIdx/
# cNtermMod/cCtermMod, with a leading "# VarModConfig: ..." comment line and a header line
# before the data) into per-chunk files matching the SAME row-count boundaries used to split
# the corresponding out_tsv (tools/run_carafe_chunked.sh's `split -l CHUNK_SIZE` on the
# headerless body). row_index values are GLOBAL/positional in the source file (0-based data-row
# position), so each chunk's extracted rows have their row_index rewritten to be LOCAL to that
# chunk's own out_tsv split (subtract chunk_index * CHUNK_SIZE) -- otherwise a downstream
# carafe_ms2_to_fi_mask.py run against a chunk-local out_tsv would look up out-of-range indices.
#
# Relies on the input being strictly row_index-ordered (true for idx_to_carafe.py's own output,
# written sequentially) so only one output chunk file needs to be open at a time -- a single
# streaming pass, never re-reading the input, and never risking "too many open files" against
# thousands of chunks.
#
# Usage: awk -v OUTDIR=dir -v CHUNK_SIZE=50000 -f split_variant_map_for_chunks.awk input.tsv

BEGIN {
    FS = "\t"; OFS = "\t"
    cur_chunk = -1
    n_rows = 0
}
NR == 1 { comment_line = $0; next }
NR == 2 { header_line = $0; next }
{
    row_index = $1 + 0
    chunk = int(row_index / CHUNK_SIZE)
    if (chunk != cur_chunk) {
        if (cur_chunk >= 0) close(outfile)
        cur_chunk = chunk
        outfile = sprintf("%s/chunk_%05d.tsv", OUTDIR, chunk)
        print comment_line > outfile
        print header_line > outfile
    }
    $1 = row_index - chunk * CHUNK_SIZE
    print > outfile
    n_rows++
}
END {
    if (cur_chunk >= 0) close(outfile)
    printf "split_variant_map_for_chunks.awk: %d data rows across %d chunk(s) (0..%d)\n", \
        n_rows, cur_chunk + 1, cur_chunk > "/dev/stderr"
}
