### Comet parameter: variable_mod01 through variable_mod15

There are 8 fields/settings that are associated with these parameters:
  - The first entry is a decimal value specifying the modification mass difference.
  - The second entry is the residue(s) that the modifications are possibly applied to.
    If more than a single residue is modified by the same mass difference, list them
    all as a string.  Use 'n' for N-terminal modification and 'c' for C-terminal modification.
  - The third entry is an integer to specify whether the modification is a
    variable modification (0) or a binary modification (non-zero value).
    Note that if you set the same binary modification value in multiple "variable_mod?" parameter
    entries, Comet will treat those variable modifications as a "binary set".  This means
    that all modifiable residues in the "binary set" must be unmodified or modified.  Multiple
    "binary sets" can be specified by setting a different binary modification value e.g.
    use "1" for all modifications in set 1, and "2" or all modifications in set 2.
    Binary modification groups were added with version 2015.02 rev. 1.
    - 0 = variable modification analyzes all permutations of modified and unmodified residues.
    - non-zero value = binary modification analyzes peptides where all residues are either modified or all residues are not modified.
  - The fourth entry is an integer specifying the maximum number of modified residues
    possible in a peptide for this modification entry. With release 2020.01 rev. 3, this
    field has been extended to allow specifying both a minimum and maximum number of
    modified residues for this modification entry. A single integer, e.g. "3", would
    specify that up to 3 variable mods are allowed.  Comma separated values, e.g. "2,4"
    would specify that peptides must have between 2 and 4 of this variable modification.
  - The fifth entry specifies the distance the modification is applied to from the respective terminus:
    - -2 = apply anywhere except c-terminal residue of peptide
    - -1 = no distance contraint
    - 0 = only applies to terminal residue
    - 1 = only applies to terminal residue and next residue
    - 2 = only applies to terminal residue through next 2 residues
    - *N* = only applies to terminal residue through next <i>N</i> residues where <i>N</i> is a positive integer
  - The sixth entry specifies which terminus the distance constraint is applied to:
    - 0 = protein N-terminus
    - 1 = protein C-terminus
    - 2 = peptide N-terminus
    - 3 = peptide C-terminus
  - The seventh entry specifies whether peptides must contain this modification.  If set to 1,
    only peptides that contain this modification will be analyzed.
    - 0 = not forced to be present
    - 1 = modification is required 
    - -1 = exclusive modification; only one of the set of exclusive modifications can appear in the peptide;
           this functionality was added with release 2024.01.0
  - The eighth entry is an optional fragment neutral loss field. For any fragment ion that
    contain the variable modification, a neutral loss will also be analyzed if the specified
    neutral loss value is not zero (0.0).  With version 2025.01.0, this field has been extended
    to accept two fragment neutral loss values.  Use a comma (no spaces) to delimit the second neutral
    loss; see example below.
  - The default value is "0.0 X 0 3 -1 0 0 0.0" if this parameter is missing *except* if Comet is
    compiled with the [Crux](http://crux.ms) flag on.
    For Crux compilation, the default value for variable_mod01 is "15.9949 M 0 3 -1 0 0 0.0" if this
    parameter is missing.

Example:
```
variable_mod01 = 15.9949 M 0 3 -1 0 0 0.0
variable_mod02 = 79.966331 STY 0 3 -1 0 0 97.976896  ... possible phosphorylation on any S, T, Y residue with a neutral loss of 98
variable_mod02 = 79.966331 STY 0 3 -1 0 0 97.976896,79.966331  ... possible phosphorylation on any S, T, Y residue with fragment neutral losses of 98 and 80 considered
variable_mod02 = 79.966331 STY 0 3 -1 0 1 0.0        ... force peptide IDs to contain at least one phosphorylation mod
variable_mod01 = 42.010565 nK 0 3 -1 0 0 0.0         ... acetylation mod to lysine and N-terminus of all peptides
variable_mod01 = 15.994915 n 0 3 0 0 0 0.0           ... oxidation of protein N-terminus
variable_mod01 = 28.0 c 0 3 8 1 0 0.0                ... modification applied to C-terminus as long as the C-term residue is one of last 9 residues in protein
variable_mod03 = -17.026549 Q 0 1 0 2 0 0.0          ... cyclization of N-terminal glutamine to form pyroglutamic acid (elimination of NH3)
variable_mod04 = -18.010565 E 0 1 0 2 0 0.0          ... cyclization of N-terminal glutamic acid to form pyroglutamic acid (elimination of H2O)
```

Here is a binary modification search example of triple SILAC plus acetylation of lysine.
The SILAC modifications are "R +6 and K +4" (medium) and "R +10 and K +8" (heavy).
In conjunction with K +42 acetylation, the binary modification sets would be
"R +6, K +4, K +4+42" for SILAC medium (binary group 1) and
"R +10, K +8, K +8+42" for SILAC heavy (binary group 2).
Mass values are listed with no precision for clarity; definitely use precise
modification masses in practice.
```
variable_mod01 = 42.0 K 0 3 -1 0 0 0.0
variable_mod02 =  6.0 R 1 3 -1 0 0 0.0
variable_mod03 =  4.0 K 1 3 -1 0 0 0.0
variable_mod04 = 46.0 K 1 3 -1 0 0 0.0
variable_mod05 = 10.0 R 2 3 -1 0 0 0.0
variable_mod06 =  8.0 K 2 3 -1 0 0 0.0
variable_mod07 = 50.0 K 2 3 -1 0 0 0.0
variable_mod08 =  0.0 X 0 3 -1 0 0 0.0
variable_mod09 =  0.0 X 0 3 -1 0 0 0.0
variable_mod10 =  0.0 X 0 3 -1 0 0 0.0
variable_mod11 =  0.0 X 0 3 -1 0 0 0.0
variable_mod12 =  0.0 X 0 3 -1 0 0 0.0
variable_mod13 =  0.0 X 0 3 -1 0 0 0.0
variable_mod14 =  0.0 X 0 3 -1 0 0 0.0
variable_mod15 =  0.0 X 0 3 -1 0 0 0.0
```

---

<div id="vmg-generator" class="vmg-container">
  <h3>Comet Variable Mod Config Generator</h3>
  <p>
    Use the form below to build a correctly formatted variable_mod line for Comet. When valid, the config string will appear in the display box and can be copied to your clipboard.
  </p>

  <div class="vmg-form">
    <div class="vmg-field">
      <label for="vmg-mass-difference">Mass Difference (float with dot):</label>
      <input type="number" id="vmg-mass-difference" step="any" value="0.0" />
      <small id="vmg-mass-difference-error" class="vmg-error" style="display:none">Please enter a valid float for mass difference (e.g., 15.9949).</small>
    </div>

    <div class="vmg-field">
      <label for="vmg-residues">Residues (Valid: n, c, A-Z):</label>
      <input type="text" id="vmg-residues" value="X" placeholder="Residues (e.g. M, STY)" title="Use 'n' for N-terminal, 'c' for C-terminal, and uppercase A-Z for residues." />
      <small id="vmg-residues-error" class="vmg-error" style="display:none">Residues contain invalid characters. Only "n", "c", and A-Z are allowed.</small>
    </div>

    <div class="vmg-field">
      <label for="vmg-modification-type">Modification Type (Variable or Binary):</label>
	  <select id="vmg-modification-type">
        <option value="0">Variable modification (0)</option>
        <option value="1">Binary modification (1); residues are either all modified or not modified</option>
      </select>
      <small id="vmg-modification-type-error" class="vmg-error" style="display:none">Please select a valid modification type.</small>
    </div>

    <div class="vmg-field">
      <label for="vmg-max-modified">Max Modified Residues (Integer or min,max):</label>
      <input type="text" id="vmg-max-modified" value="3" />
      <small id="vmg-max-modified-error" class="vmg-error" style="display:none">Please enter a valid integer or min,max (e.g., 2 or 2,4).</small>
    </div>

    <div class="vmg-field">
      <label for="vmg-distance">Distance from Terminus:</label>
      <select id="vmg-distance">
        <option value="-2">Apply anywhere except C-terminal residue of peptide (-2)</option>
        <option value="-1" selected>No distance constraint (-1)</option>
        <option value="0">Only applies to terminal residue (0)</option>
        <option value="1">Only applies to terminal residue and next residue (1)</option>
        <option value="2">Only applies to terminal residue through next 2 residues (2)</option>
        <option value="3">Only applies to terminal residue through next 3 residues (3)</option>
        <option value="4">Only applies to terminal residue through next 4 residues (4)</option>
        <option value="5">Only applies to terminal residue through next 5 residues (5)</option>
        <option value="6">Only applies to terminal residue through next 6 residues (6)</option>
        <option value="7">Only applies to terminal residue through next 7 residues (7)</option>
        <option value="8">Only applies to terminal residue through next 8 residues (8)</option>
        <option value="9">Only applies to terminal residue through next 9 residues (9)</option>
      </select>
      <small id="vmg-distance-error" class="vmg-error" style="display:none">Please select a valid distance option.</small>
    </div>

    <div class="vmg-field">
      <label for="vmg-terminus">Terminus:</label>
      <select id="vmg-terminus">
        <option value="0">Protein N-terminus (0)</option>
        <option value="1">Protein C-terminus (1)</option>
        <option value="2">Peptide N-terminus (2)</option>
        <option value="3">Peptide C-terminus (3)</option>
      </select>
    </div>

    <div class="vmg-field">
      <label for="vmg-mod-required">Modification Required:</label>
	  <select id="vmg-mod-required">
        <option value="0">this variable modification not forced to be present (0)</option>
        <option value="1">this variable modification is required to be present (1)</option>
        <option value="-1">this is an exclusive modification (-1)</option>
      </select>
    </div>

    <div class="vmg-field">
      <label for="vmg-neutral-loss">Fragment Neutral Loss (single float or two floats comma separated no spaces):</label>
      <input type="text" id="vmg-neutral-loss" value="0.0" />
      <small id="vmg-neutral-loss-error" class="vmg-error" style="display:none">
        Enter either a single float ("97.976896") or two floats comma-separated with NO spaces ("97.976896,79.966331"). Enter "0.0" for no fragment neutral loss.
      </small>
    </div>

    <div style="margin-top:0.5em;">
      <span style="font-weight:bold">variable_modXX =</span>
      <div id="vmg-config-string" style="display:inline-block; border:1px solid #ccc; padding:8px; margin-left:0.5em; font-weight:bold; min-width:320px;">&nbsp;</div>
    </div>

    <div style="margin-top:0.5em;">
      <button id="vmg-copy-btn" disabled style="padding:8px 12px; background:#007bff; color:white; border:none; border-radius:4px; cursor:pointer;">Copy Config String</button>
      <span style="margin-left:1em; color:#666; font-size:0.9em;">(Copied string is plain text suitable to paste into a Comet .params file.)</span>
    </div>
  </div>
</div>

<style>
  /* Scoped styles for the variable mod generator */
  #vmg-generator { font-size:12px; background:#fff; padding:12px; border-radius:6px; border:1px solid #e1e4e8; box-shadow:0 1px 0 rgba(0,0,0,0.03); max-width:880px; }
  #vmg-generator h3 { margin-top:0; }
  #vmg-generator .vmg-field { margin-bottom:0.8em; }
  #vmg-generator label { display:block; font-weight:600; margin-bottom:0.25em; }
  #vmg-generator input[type="text"], #vmg-generator input[type="number"], #vmg-generator select { width:100%; padding:7px; border:1px solid #d0d7de; border-radius:4px; }
  #vmg-generator .vmg-error { color:#b31d1d; font-size:0.85em; }
</style>

<script>
(function () {
  // Scoped helper: only operate inside the generator container
  const root = document.getElementById('vmg-generator');
  if (!root) return;

  const elems = {
    massDifference: root.querySelector('#vmg-mass-difference'),
    massError: root.querySelector('#vmg-mass-difference-error'),
    residues: root.querySelector('#vmg-residues'),
    residuesError: root.querySelector('#vmg-residues-error'),
    modType: root.querySelector('#vmg-modification-type'),
    modTypeError: root.querySelector('#vmg-modification-type-error'),
    maxModified: root.querySelector('#vmg-max-modified'),
    maxModifiedError: root.querySelector('#vmg-max-modified-error'),
    distance: root.querySelector('#vmg-distance'),
    distanceError: root.querySelector('#vmg-distance-error'),
    terminus: root.querySelector('#vmg-terminus'),
    modRequired: root.querySelector('#vmg-mod-required'),
    neutralLoss: root.querySelector('#vmg-neutral-loss'),
    neutralLossError: root.querySelector('#vmg-neutral-loss-error'),
    configDisplay: root.querySelector('#vmg-config-string'),
    copyBtn: root.querySelector('#vmg-copy-btn')
  };

	// Regex to enforce:
	// - single float that MUST have a decimal point (optionally leading -)
	// - OR two floats separated by a single comma with NO spaces
	// Examples valid: 97.976896  ;  97.976896,79.966331
	const neutralLossPattern = /^-?\d+\.\d+(,-?\d+\.\d+)?$/;
	
  function validateInputs() {
    let isValid = true;

    // Validate mass difference: require numeric value (allow negative), must include a dot or be a valid float
    const massVal = elems.massDifference.value;
    const massNumeric = parseFloat(massVal);
    if (massVal === "" || Number.isNaN(parseFloat(massNumeric)) || !/^-?\d+(\.\d+)$/.test(massVal)) {
      elems.massError.style.display = 'inline';
      isValid = false;
    } else {
      elems.massError.style.display = 'none';
    }

    // Validate residues: allow 'n' and 'c' (lowercase) and uppercase A-Z only
    const residuesVal = (elems.residues.value || '').trim();
    if (!/^[ncA-Z]+$/.test(residuesVal)) {
      elems.residuesError.style.display = 'inline';
      isValid = false;
    } else {
      elems.residuesError.style.display = 'none';
    }

    // Validate maxModified: allow integer or comma-separated integer pair "min,max"
    const maxVal = (elems.maxModified.value || '').trim();
    if (!/^\d+(\,\d+)?$/.test(maxVal)) {
      elems.maxModifiedError.style.display = 'inline';
      isValid = false;
    } else {
      elems.maxModifiedError.style.display = 'none';
    }

    // neutral loss: enforce single float or two floats comma separated, no spaces
    const nl = (elems.neutralLoss.value || '').trim();
    if (nl === "") {
      // Empty is treated as the default neutral loss "0.0" (valid).
      elems.neutralLossError.style.display = 'none';
    } else if (!neutralLossPattern.test(nl)) {
      elems.neutralLossError.style.display = 'inline';
      isValid = false;
    } else {
      elems.neutralLossError.style.display = 'none';
    }
	  
    // distance always valid from select; placeholder error not used except for future checks
    return isValid;
  }

  function updateConfigString() {
    if (!validateInputs()) {
      elems.configDisplay.textContent = '';
      elems.copyBtn.disabled = true;
      return;
    }

    const parts = [
      elems.massDifference.value.trim(),
      elems.residues.value.trim(),
      elems.modType.value.trim(),
      elems.maxModified.value.trim(),
      elems.distance.value,
      elems.terminus.value,
      elems.modRequired.value.trim(),
      // If neutralLoss is empty, use default "0.0"
      (elems.neutralLoss.value && elems.neutralLoss.value.trim() !== '') ? elems.neutralLoss.value.trim() : '0.0'
	];

    const configString = parts.join(' ');
    elems.configDisplay.textContent = configString;
    elems.copyBtn.disabled = false;
  }

  function copyConfig() {
    const text = elems.configDisplay.textContent || '';
    if (!text) return;
    // Try Clipboard API first
    if (navigator.clipboard && navigator.clipboard.writeText) {
      navigator.clipboard.writeText(text).then(function () {
        // Provide lightweight feedback
        const orig = elems.copyBtn.textContent;
        elems.copyBtn.textContent = 'Copied!';
        setTimeout(() => elems.copyBtn.textContent = orig, 1200);
      }).catch(function () {
        fallbackCopy(text);
      });
    } else {
      fallbackCopy(text);
    }
  }

  function fallbackCopy(text) {
    const ta = document.createElement('textarea');
    ta.value = text;
    document.body.appendChild(ta);
    ta.select();
    try {
      document.execCommand('copy');
      const orig = elems.copyBtn.textContent;
      elems.copyBtn.textContent = 'Copied!';
      setTimeout(() => elems.copyBtn.textContent = orig, 1200);
    } catch (e) {
      alert('Copy failed — please select and copy manually.');
    }
    document.body.removeChild(ta);
  }

  // Attach listeners
  const watchEls = root.querySelectorAll('input, select');
  watchEls.forEach(function (el) {
    el.addEventListener('input', updateConfigString);
    el.addEventListener('change', updateConfigString);
  });

  elems.copyBtn.addEventListener('click', copyConfig);

  // Initial populate
  updateConfigString();
})();
</script>

<!-- End of generator section -->





