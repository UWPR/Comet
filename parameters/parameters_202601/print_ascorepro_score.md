### Comet parameter: print_ascorepro_score

- This parameter determines whether or not the AScorePro algorithm is applied
to the search results.
- The AScorePro algorithm is only applied to the top ranked hit for each spectrum query.
- If it is applied, the AScorePro MOB score will be available in the real-time search
interface as well as .txt, .pep.xml, and .mzid output formats.
- In the .txt output, the AScorePro MOB score is reported in column titled "ascorepro".
- In the .pep.xml output, the AScorePro MOB score is reported as a search score using 
the "ascorepro_score" attribute. The site scores are reported as a search score using
the "ascorepro_sitescore" attribute.
- In the .mzid output, the AScorePro MOB score is reported as a search score using the
cvParam element with the following attributes:
cvRef="PSI-MS" accession="MS:1001968" name="PTM localization PSM-level statistic" value="X.XXXX".
- Both the MOB score and site scores are returned through the CometWrapper real-time
search interface.
- AScorePro MOB scores are a decimal value whereas site scores are reported as
a string composed of space separate pairs of "position":"score". An examplie site score
might be "1:13.86 5:15.94" for the peptide "QAS[79.9663]ES[79.9663]K".
- When the AScorePro algorithm is applied, any peptide with a MOB score >= 13 will have
its localized peptidoform replace Comet's top ranked peptide. This means that the
new top ranked peptidoform might be a duplicate of a lower ranked result.
- Note that the maximum site score is 5000.0 if the variable modification is on the only
residue in the peptide that can be modified. Negative and zero site scores are possible;
I suggest you contact [the AScorePro devs](https://github.com/gygilab/MPToolkit) for
further details of their algorithm.
- Valid values are
  - "-1" to localize all variable modifications in a peptide
  - "0" to not apply the AScorePro algorithm
  - "1" to localize the modification specified by variable_mod01
  - "2" to localize the modification specified by variable_mod02
  - "3" to localize the modification specified by variable_mod03
  - "4" to localize the modification specified by variable_mod04
  - "5" to localize the modification specified by variable_mod05
- The default value is "0" if this parameter is missing.
- Note that this is currently considered an experimental score in Comet. Especially the
extension to localize all modifications in a peptide (or any modification that is not
phosphoprylation per the (AScorePro publication)[https://pubmed.ncbi.nlm.nih.gov/36280721/]).
The meaning or utility of the AScorePro MOB score to localize non-phosphorylation
modifications will need to be validated.

Example:
```
print_expect_score = 0     (do not run AScorePro)
print_expect_score = 1     (to localize the variable_mod01 mods)
print_expect_score = 3     (to localize the variable_mod03 mods)
print_expect_score = -1    (to localize all variable mods in a peptide)
```
