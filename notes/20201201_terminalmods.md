### Notes 2020.12.01

Comet's terminal modification reporting

- Back in the early 2000s during the development of the pepXML format, reported
n- and c-terminal modification masses would be reported with the addition of H
and OH, respectively. This was based on how I these masses were handled in the
search code.
- So an acetylated n-terminus (+42) would have its peptide string be reported
as something like "n[43]DLASTK".
- Similarly any c-terminal modification would be reported with an additional
+17 mass.

