### Notes 2024/11/12: AScorePro integration in Comet

As of Comet release v2025.03 rev. 0, the AScorePro site localization algorithm has
been integrated into the program.  Comet's AScorePro code is based on translating
the C# code from https://github.com/gygilab/MPToolkit to C++.


Until noted otherwise, this is considered an experimental
feature as I still need to spend more time to validate that the ASCorePro MOB scores
generated in Comet are valid, especially those from our custom extension to allow
localization on all variable modifications on a peptide and not just for phosphorylation.

I want to thank V. Sharma for translating the code to C++ and extending the scoring
to all modifications in a peptide.

### How to run AScorePro in Comet
- Use the [print_ascorepro_score](https://uwpr.github.io/Comet/parameters/parameters_202503/print_ascorepro_score.html) parameter to control how to apply AScorePro in Comet.
- Site localization can be specified to happen on any one variable modification (e.g. set the parameter value to "1" to localization the modification specified by variable_mod01 or set the parameter value to "3" to localize the modification specified by variable_mod03).
- Site localization can be specified to happen on all variable modifications in a peptide by setting the parameter value to "-1".
- Site localization is not performed if this parameter is missing for the parameter value is set to "0".

### Application of AScorePro both real-time and standard (offline) searches

- Controlled by the [print_ascore_pro](https://uwpr.github.io/Comet/parameters/parameters_202503/print_ascorepro_score.html) search parameter.
- Applies to top hit only.
- Replace Comet’s top hit peptide with AScorePro localized peptide only if AScorePro MOB score is 13 or greater.
  - This means it’s possible that the 2nd or lower peptide in the output list can be the same as the new/replaced top hit.
- Report AScorePro score (and possibly site scores) for the top hit is 13 or greater.

### Standard Comet command line (aka offline) search output
The AScorePro results are currently exported in the .txt, .pep.xml and.mzid output formats.
- .txt output as additional columns:
  - “ascorepro_score” (as a single floating point number)
  - “ascorepro_site_score” (as space separated “position:site_score” pairs e.g. “6:3.28 10:41.53”)
- .pep.xml output as additional search scores:
  - &lt;search_score name=”ascorepro” value=”129.3530”/&gt;
  - &lt;search_score name=”ascorepro_sitescore” value=”4:38.4034 12:0.0000”/&gt;
- .mzid output as an additional search score:
   - &lt;vParam cvRef="PSI-MS" accession="MS:1001968" name="PTM localization PSM-level statistic" value="129.3530"/&gt;

### Real-time search output
- Report AScorePro score (ScoreWrapper .dAScoreScore)
- Report site score as a string composed of space separated “position:score” pairs e.g. “6:3.28 10:41.53”.  (ScoreWrapper .sAScoreProSiteScores).
   
The chart below shows RTS search times for a single RTS run with or without AScorePro.  The timings show that
the AScorePro algorithm imparts negligible costs in terms of per-spectrum-query run times.  Any differences are
minor and can be attributed to standard run-to-run variability.

![AScorePro timing](https://uwpr.github.io/Comet/notes/20251112-AScoreProTiming.png)