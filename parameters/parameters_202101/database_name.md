### Comet parameter: database_name

- A full or relative path to the sequence database, in FASTA format, to search. Example databases
include RefSeq or UniProt.
- Database can contain amino acid sequences or nucleic acid sequences.  If sequences are amino acid
sequences, set the parameter "nucleotide_reading_frame = 0".  If the sequences are nucleic acid
sequences, you must instruct Comet to translate these to amino acid sequences.  Do this by setting
"nucleotide_reading_frame" to a value between 1 and 9.
- Databases can also be Comet indexed peptide database format (with .idx extension), currently
intended for real-time, on-the-fly searches only.
- There is no default value if this parameter is missing.

Example:
```
database_name = yeast.fasta
database_name = C:\local\db\yeast.fasta
database_name = /usr/local/db/yeast.fasta
```
