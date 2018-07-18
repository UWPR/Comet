using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using CometWrapper;

using System.Runtime.InteropServices;

namespace RealtimeSearch
{
   class Search
   {
      static void Main()
      {
         String sTmp;
         int iTmp;
         double dTmp;

         CometSearchManagerWrapper SearchMgr;
         SearchMgr = new CometSearchManagerWrapper();
         // can set other parameters this way
         string sDB = "C:\\Users\\Jimmy\\Desktop\\work\\cometsearch\\human.fasta.idx";
//         string sDB = "C:\\Users\\Jimmy\\Desktop\\devin\\YEAST_IT_400-10000.fasta.idx";
         SearchMgr.SetParam("database_name", sDB, sDB);

         iTmp = 0; // 0=no, 1=concatenated, 2=separate
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("decoy_search", sTmp, iTmp);

         dTmp = 100.0; //ppm window
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("peptide_mass_tolerance", sDB, dTmp);

         iTmp = 2; // 0=Da, 2=ppm
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("peptide_mass_units", sTmp, iTmp);

         iTmp = 1; // 1=monoisotopic, do not change
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("mass_type_parent", sTmp, iTmp);

         iTmp = 1; // 1=monoisotopic, do not change
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("mass_type_fragment", sTmp, iTmp);

         iTmp = 1; // m/z tolerance
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("precursor_tolerance_type", sTmp, iTmp);

         iTmp = 3; // 0=off, 1=0/1 (C13 error), 2=0/1/2, 3=0/1/2/3, 4=-8/-4/0/4/8 (for +4/+8 labeling)
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("isotope_error", sTmp, iTmp);

         iTmp = 0; // 0=no, 1=yes
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("require_variable_mod", sTmp, iTmp);

         dTmp = 1.0005; // fragment bin width
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("fragment_bin_tol", sTmp, dTmp);

         dTmp = 0.4; // fragment bin offset
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("fragment_bin_offset", sTmp, dTmp);

         iTmp = 1; // 0=use flanking peaks, 1=M peak only
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("theoretical_fragment_ions", sTmp, iTmp);

         iTmp = 3;
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("max_fragment_charge", sTmp, iTmp);

         iTmp = 6;
         sTmp = iTmp.ToString();
         SearchMgr.SetParam("max_precursor_charge", sTmp, iTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_Cterm_peptide", sTmp, dTmp);

         dTmp = 229.162932;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_Nterm_peptide", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_Cterm_protein", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_Nterm_protein", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_G_glucine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_A_alanine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_S_serine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_P_proline", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_V_valine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_T_threonine", sTmp, dTmp);

         dTmp = 57.0214637236;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_C_cysteine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_L_leucine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_I_isoleucine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_N_asparagine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_D_aspartic_acid", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_Q_glutamine", sTmp, dTmp);

         dTmp = 229.162932;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_K_lysine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_E_glutamic_acid", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_M_methionine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_O_ornithine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_H_histidine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_F_phenylalanine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_U_selenocysteine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_R_arginine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_Y_tyrosine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_W_tryptophan", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_Y_tyrosine", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_B_user_amino_acid", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_J_user_amino_acid", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_X_user_amino_acid", sTmp, dTmp);

         dTmp = 0.0;
         sTmp = dTmp.ToString();
         SearchMgr.SetParam("add_Z_user_amino_acid", sTmp, dTmp); 

         string modString = "M,15.9949146221,0,3,-1,0,0";
         var varModsWrapper = new VarModsWrapper();
         varModsWrapper.set_VarModChar("M");
         varModsWrapper.set_VarModMass(15.9949146221);
         varModsWrapper.set_BinaryMod(0);
         varModsWrapper.set_MaxNumVarModAAPerMod(3);  // allow up to 3 of these mods in peptide
         varModsWrapper.set_VarModTermDistance(-1);   // allow mod to be anywhere in peptide
         varModsWrapper.set_WhichTerm(0);             // unused if -1 set above; 0=protein terminus reference
         varModsWrapper.set_RequireThisMod(0);        // 0=not required, 1=required
         SearchMgr.SetParam("variable_mod01", modString, varModsWrapper);   

         // need precursor charge, precursor m/z and peaklist to do search
         int iPrecursorCharge = 3;
         double dMZ = 562.012;
         int iNumPeaks = 318;                         // number of peaks in spectrum
         double[] pdMass = new double[iNumPeaks];   // stores mass of spectral peaks
         double[] pdInten = new double[iNumPeaks];  // stores inten of spectral peaks
         
         // now populate the spectrum as mass and intensity arrays from whatever is retrieved from Thermo interface
         // example below is y-ions from human tryptic peptide EAGAQAVPET[79.9663]R
         pdMass[0] = 120.0808; pdInten[0] = 3636.832;
         pdMass[1] = 124.673; pdInten[1] = 1093.574;
         pdMass[2] = 126.1276; pdInten[2] = 6416.095;
         pdMass[3] = 127.1247; pdInten[3] = 5836.08;
         pdMass[4] = 127.131; pdInten[4] = 7041.313;
         pdMass[5] = 128.128; pdInten[5] = 7306.506;
         pdMass[6] = 128.1344; pdInten[6] = 7074.71;
         pdMass[7] = 129.1314; pdInten[7] = 7140.029;
         pdMass[8] = 129.1378; pdInten[8] = 7143.752;
         pdMass[9] = 130.1348; pdInten[9] = 6912.803;
         pdMass[10] = 130.1411; pdInten[10] = 6524.468;
         pdMass[11] = 131.1381; pdInten[11] = 3952.705;
         pdMass[12] = 131.1444; pdInten[12] = 2007.381;
         pdMass[13] = 138.1219; pdInten[13] = 1224.884;
         pdMass[14] = 140.0637; pdInten[14] = 1022.291;
         pdMass[15] = 140.7158; pdInten[15] = 979.7787;
         pdMass[16] = 142.1053; pdInten[16] = 998.3242;
         pdMass[17] = 143.4036; pdInten[17] = 972.4504;
         pdMass[18] = 144.5373; pdInten[18] = 1120.372;
         pdMass[19] = 146.9177; pdInten[19] = 985.8047;
         pdMass[20] = 147.4706; pdInten[20] = 1168.35;
         pdMass[21] = 147.4778; pdInten[21] = 953.5622;
         pdMass[22] = 147.4806; pdInten[22] = 1258.757;
         pdMass[23] = 147.4865; pdInten[23] = 1562.013;
         pdMass[24] = 147.4986; pdInten[24] = 2466.522;
         pdMass[25] = 147.5056; pdInten[25] = 4533.226;
         pdMass[26] = 147.526; pdInten[26] = 6670.509;
         pdMass[27] = 147.5357; pdInten[27] = 4489.807;
         pdMass[28] = 147.545; pdInten[28] = 1770.461;
         pdMass[29] = 147.5545; pdInten[29] = 1626.731;
         pdMass[30] = 147.5633; pdInten[30] = 1082.199;
         pdMass[31] = 147.5735; pdInten[31] = 1118.768;
         pdMass[32] = 147.581; pdInten[32] = 905.918;
         pdMass[33] = 158.0923; pdInten[33] = 1485.275;
         pdMass[34] = 167.0814; pdInten[34] = 3118.248;
         pdMass[35] = 172.0717; pdInten[35] = 1746.7;
         pdMass[36] = 173.1497; pdInten[36] = 2139.939;
         pdMass[37] = 175.1189; pdInten[37] = 8591.988;
         pdMass[38] = 178.2667; pdInten[38] = 17442.43;
         pdMass[39] = 184.1079; pdInten[39] = 1440.853;
         pdMass[40] = 185.1649; pdInten[40] = 1528.001;
         pdMass[41] = 195.0762; pdInten[41] = 1479.49;
         pdMass[42] = 204.0868; pdInten[42] = 1671.248;
         pdMass[43] = 211.1439; pdInten[43] = 2163.664;
         pdMass[44] = 211.6472; pdInten[44] = 1276.858;
         pdMass[45] = 212.1029; pdInten[45] = 12824.37;
         pdMass[46] = 213.087; pdInten[46] = 1628.273;
         pdMass[47] = 214.7324; pdInten[47] = 1441.302;
         pdMass[48] = 215.1388; pdInten[48] = 2605.684;
         pdMass[49] = 226.0818; pdInten[49] = 1738.171;
         pdMass[50] = 228.1344; pdInten[50] = 4003.042;
         pdMass[51] = 230.1702; pdInten[51] = 24777.45;
         pdMass[52] = 234.1235; pdInten[52] = 1731.768;
         pdMass[53] = 235.119; pdInten[53] = 1827.138;
         pdMass[54] = 239.1141; pdInten[54] = 2468.418;
         pdMass[55] = 240.0977; pdInten[55] = 1936.423;
         pdMass[56] = 243.134; pdInten[56] = 2310.606;
         pdMass[57] = 244.0923; pdInten[57] = 1528.802;
         pdMass[58] = 248.1809; pdInten[58] = 2856.698;
         pdMass[59] = 258.1087; pdInten[59] = 2412.593;
         pdMass[60] = 258.1237; pdInten[60] = 2155.257;
         pdMass[61] = 261.1259; pdInten[61] = 1561.772;
         pdMass[62] = 262.1187; pdInten[62] = 2192.51;
         pdMass[63] = 262.1508; pdInten[63] = 5001.569;
         pdMass[64] = 283.1043; pdInten[64] = 2931.259;
         pdMass[65] = 284.0879; pdInten[65] = 1580.954;
         pdMass[66] = 300.1553; pdInten[66] = 3220.081;
         pdMass[67] = 301.1145; pdInten[67] = 3209.384;
         pdMass[68] = 315.2584; pdInten[68] = 2540.354;
         pdMass[69] = 319.6944; pdInten[69] = 2509.718;
         pdMass[70] = 320.1957; pdInten[70] = 1702.092;
         pdMass[71] = 328.1864; pdInten[71] = 1536.453;
         pdMass[72] = 331.1512; pdInten[72] = 2229.812;
         pdMass[73] = 333.1916; pdInten[73] = 1530.389;
         pdMass[74] = 333.6925; pdInten[74] = 15441.13;
         pdMass[75] = 334.1936; pdInten[75] = 4028.567;
         pdMass[76] = 342.1422; pdInten[76] = 2440.415;
         pdMass[77] = 342.2044; pdInten[77] = 2556.539;
         pdMass[78] = 342.3268; pdInten[78] = 1694.012;
         pdMass[79] = 343.1615; pdInten[79] = 5551.158;
         pdMass[80] = 343.2547; pdInten[80] = 4771.969;
         pdMass[81] = 345.1046; pdInten[81] = 3644.229;
         pdMass[82] = 349.2437; pdInten[82] = 4433.525;
         pdMass[83] = 359.1704; pdInten[83] = 4317.669;
         pdMass[84] = 369.1773; pdInten[84] = 1757.155;
         pdMass[85] = 376.1949; pdInten[85] = 2154.578;
         pdMass[86] = 376.2764; pdInten[86] = 8802.36;
         pdMass[87] = 376.7173; pdInten[87] = 3071.063;
         pdMass[88] = 377.2389; pdInten[88] = 11841.12;
         pdMass[89] = 378.1669; pdInten[89] = 1461.972;
         pdMass[90] = 378.2426; pdInten[90] = 1741.25;
         pdMass[91] = 381.7086; pdInten[91] = 3170.129;
         pdMass[92] = 385.2556; pdInten[92] = 1701.473;
         pdMass[93] = 390.212; pdInten[93] = 1876.061;
         pdMass[94] = 390.7139; pdInten[94] = 45101.01;
         pdMass[95] = 391.2151; pdInten[95] = 15922.55;
         pdMass[96] = 391.7174; pdInten[96] = 2327.207;
         pdMass[97] = 394.1365; pdInten[97] = 1951.905;
         pdMass[98] = 400.2766; pdInten[98] = 3316.227;
         pdMass[99] = 401.2867; pdInten[99] = 2798.067;
         pdMass[100] = 412.147; pdInten[100] = 2727.461;
         pdMass[101] = 413.2504; pdInten[101] = 5357.341;
         pdMass[102] = 424.7461; pdInten[102] = 2939.362;
         pdMass[103] = 429.1736; pdInten[103] = 1790.45;
         pdMass[104] = 430.1575; pdInten[104] = 2184.096;
         pdMass[105] = 430.2869; pdInten[105] = 1699.388;
         pdMass[106] = 430.7369; pdInten[106] = 1348.8;
         pdMass[107] = 433.2586; pdInten[107] = 9901.589;
         pdMass[108] = 433.7587; pdInten[108] = 2542.501;
         pdMass[109] = 442.2284; pdInten[109] = 1929.097;
         pdMass[110] = 444.0966; pdInten[110] = 1379.164;
         pdMass[111] = 447.2562; pdInten[111] = 12139.36;
         pdMass[112] = 447.757; pdInten[112] = 5790.704;
         pdMass[113] = 457.2283; pdInten[113] = 1358.393;
         pdMass[114] = 468.1909; pdInten[114] = 1681.225;
         pdMass[115] = 473.2143; pdInten[115] = 2327.424;
         pdMass[116] = 473.3271; pdInten[116] = 1449.23;
         pdMass[117] = 480.2201; pdInten[117] = 1870.595;
         pdMass[118] = 480.778; pdInten[118] = 1444.358;
         pdMass[119] = 489.2789; pdInten[119] = 3672.251;
         pdMass[120] = 495.7643; pdInten[120] = 1694.642;
         pdMass[121] = 496.257; pdInten[121] = 8119.981;
         pdMass[122] = 496.7578; pdInten[122] = 3606.96;
         pdMass[123] = 501.3243; pdInten[123] = 3145.413;
         pdMass[124] = 504.2686; pdInten[124] = 6477.96;
         pdMass[125] = 504.7697; pdInten[125] = 116499.6;
         pdMass[126] = 505.2711; pdInten[126] = 46729.73;
         pdMass[127] = 505.7722; pdInten[127] = 9640.549;
         pdMass[128] = 526.3353; pdInten[128] = 7411.057;
         pdMass[129] = 529.3187; pdInten[129] = 19505.28;
         pdMass[130] = 530.2369; pdInten[130] = 1914.754;
         pdMass[131] = 530.3223; pdInten[131] = 5703.631;
         pdMass[132] = 537.376; pdInten[132] = 1688.196;
         pdMass[133] = 540.3066; pdInten[133] = 11359.36;
         pdMass[134] = 540.8079; pdInten[134] = 5185.226;
         pdMass[135] = 541.1904; pdInten[135] = 1480.036;
         pdMass[136] = 554.3038; pdInten[136] = 28780.3;
         pdMass[137] = 554.8053; pdInten[137] = 15028.03;
         pdMass[138] = 579.4243; pdInten[138] = 1773.535;
         pdMass[139] = 582.382; pdInten[139] = 1572.154;
         pdMass[140] = 590.326; pdInten[140] = 3090.674;
         pdMass[141] = 601.3483; pdInten[141] = 5123.943;
         pdMass[142] = 601.8471; pdInten[142] = 4495.191;
         pdMass[143] = 610.8466; pdInten[143] = 5756.543;
         pdMass[144] = 611.4224; pdInten[144] = 1721.227;
         pdMass[145] = 621.4066; pdInten[145] = 2169.513;
         pdMass[146] = 627.2509; pdInten[146] = 1911.659;
         pdMass[147] = 627.383; pdInten[147] = 36647.84;
         pdMass[148] = 628.3857; pdInten[148] = 9998.146;
         pdMass[149] = 636.0046; pdInten[149] = 1569.35;
         pdMass[150] = 638.384; pdInten[150] = 4460.787;
         pdMass[151] = 639.4197; pdInten[151] = 5668.544;
         pdMass[152] = 642.3718; pdInten[152] = 1477.545;
         pdMass[153] = 657.8909; pdInten[153] = 3522.903;
         pdMass[154] = 666.378; pdInten[154] = 17160.21;
         pdMass[155] = 667.3805; pdInten[155] = 2634.269;
         pdMass[156] = 673.4031; pdInten[156] = 4435.823;
         pdMass[157] = 674.407; pdInten[157] = 2462.945;
         pdMass[158] = 675.3672; pdInten[158] = 6624.378;
         pdMass[159] = 675.8668; pdInten[159] = 3263.174;
         pdMass[160] = 684.8976; pdInten[160] = 2255.424;
         pdMass[161] = 703.8762; pdInten[161] = 2269.159;
         pdMass[162] = 713.4121; pdInten[162] = 1537.77;
         pdMass[163] = 714.9344; pdInten[163] = 2450.862;
         pdMass[164] = 715.3978; pdInten[164] = 2503.357;
         pdMass[165] = 721.4183; pdInten[165] = 1636.826;
         pdMass[166] = 722.4576; pdInten[166] = 2783.057;
         pdMass[167] = 727.4233; pdInten[167] = 2104.665;
         pdMass[168] = 740.4685; pdInten[168] = 2698.945;
         pdMass[169] = 741.3884; pdInten[169] = 2314.222;
         pdMass[170] = 742.4019; pdInten[170] = 1759.336;
         pdMass[171] = 752.4258; pdInten[171] = 4287.784;
         pdMass[172] = 752.5002; pdInten[172] = 2413.001;
         pdMass[173] = 755.9526; pdInten[173] = 5270.68;
         pdMass[174] = 756.4525; pdInten[174] = 4434.838;
         pdMass[175] = 758.4163; pdInten[175] = 14512.2;
         pdMass[176] = 759.4188; pdInten[176] = 5929.451;
         pdMass[177] = 760.9007; pdInten[177] = 2042.959;
         pdMass[178] = 761.3985; pdInten[178] = 2027.361;
         pdMass[179] = 773.4797; pdInten[179] = 1821.06;
         pdMass[180] = 780.4213; pdInten[180] = 14080.47;
         pdMass[181] = 781.4204; pdInten[181] = 3927.046;
         pdMass[182] = 786.487; pdInten[182] = 2550.142;
         pdMass[183] = 790.974; pdInten[183] = 1676.443;
         pdMass[184] = 801.4653; pdInten[184] = 3763.471;
         pdMass[185] = 802.4204; pdInten[185] = 1754.715;
         pdMass[186] = 812.4945; pdInten[186] = 6685.94;
         pdMass[187] = 812.9953; pdInten[187] = 9445.543;
         pdMass[188] = 825.42; pdInten[188] = 6539.703;
         pdMass[189] = 825.9183; pdInten[189] = 3438.758;
         pdMass[190] = 826.4257; pdInten[190] = 1789.893;
         pdMass[191] = 832.4201; pdInten[191] = 2981.784;
         pdMass[192] = 833.4205; pdInten[192] = 2270.338;
         pdMass[193] = 833.9362; pdInten[193] = 3278.119;
         pdMass[194] = 834.4368; pdInten[194] = 2034.551;
         pdMass[195] = 844.4957; pdInten[195] = 2504.376;
         pdMass[196] = 844.9889; pdInten[196] = 1954.47;
         pdMass[197] = 845.4476; pdInten[197] = 13078.94;
         pdMass[198] = 845.4995; pdInten[198] = 2587.159;
         pdMass[199] = 846.4507; pdInten[199] = 3125.054;
         pdMass[200] = 851.4034; pdInten[200] = 1571.046;
         pdMass[201] = 859.5159; pdInten[201] = 2192.929;
         pdMass[202] = 865.5107; pdInten[202] = 2486.068;
         pdMass[203] = 889.4531; pdInten[203] = 2696.819;
         pdMass[204] = 889.952; pdInten[204] = 4803.624;
         pdMass[205] = 890.4536; pdInten[205] = 2247.167;
         pdMass[206] = 893.5057; pdInten[206] = 8322.581;
         pdMass[207] = 894.5059; pdInten[207] = 4370.521;
         pdMass[208] = 899.5682; pdInten[208] = 2784.964;
         pdMass[209] = 914.5484; pdInten[209] = 1627.606;
         pdMass[210] = 927.0294; pdInten[210] = 6840.504;
         pdMass[211] = 927.5288; pdInten[211] = 5673.203;
         pdMass[212] = 932.4805; pdInten[212] = 24245.57;
         pdMass[213] = 933.4822; pdInten[213] = 13199.91;
         pdMass[214] = 934.488; pdInten[214] = 3631.166;
         pdMass[215] = 953.9844; pdInten[215] = 1636.78;
         pdMass[216] = 959.0278; pdInten[216] = 2160.621;
         pdMass[217] = 963.4803; pdInten[217] = 2080.067;
         pdMass[218] = 973.4545; pdInten[218] = 1858.422;
         pdMass[219] = 975.6357; pdInten[219] = 1575.601;
         pdMass[220] = 983.611; pdInten[220] = 2631.567;
         pdMass[221] = 984.5421; pdInten[221] = 5475.261;
         pdMass[222] = 985.0497; pdInten[222] = 2740.656;
         pdMass[223] = 985.5418; pdInten[223] = 3052.812;
         pdMass[224] = 990.6077; pdInten[224] = 3091.949;
         pdMass[225] = 991.5096; pdInten[225] = 1751.271;
         pdMass[226] = 994.5497; pdInten[226] = 1776.421;
         pdMass[227] = 1008.532; pdInten[227] = 31106.21;
         pdMass[228] = 1009.534; pdInten[228] = 13191.75;
         pdMass[229] = 1010.538; pdInten[229] = 3120.793;
         pdMass[230] = 1016.549; pdInten[230] = 2157.28;
         pdMass[231] = 1017.539; pdInten[231] = 2026.99;
         pdMass[232] = 1027.554; pdInten[232] = 2531.909;
         pdMass[233] = 1036.604; pdInten[233] = 2090.619;
         pdMass[234] = 1041.586; pdInten[234] = 2871.713;
         pdMass[235] = 1045.564; pdInten[235] = 27779.68;
         pdMass[236] = 1046.567; pdInten[236] = 14258.44;
         pdMass[237] = 1047.566; pdInten[237] = 5335.381;
         pdMass[238] = 1069.057; pdInten[238] = 1700.213;
         pdMass[239] = 1079.605; pdInten[239] = 2880.457;
         pdMass[240] = 1084.548; pdInten[240] = 1903.548;
         pdMass[241] = 1092.105; pdInten[241] = 1715.751;
         pdMass[242] = 1101.571; pdInten[242] = 2977.059;
         pdMass[243] = 1102.573; pdInten[243] = 2146.502;
         pdMass[244] = 1107.601; pdInten[244] = 16676.93;
         pdMass[245] = 1108.604; pdInten[245] = 8278.275;
         pdMass[246] = 1140.632; pdInten[246] = 2221.572;
         pdMass[247] = 1143.584; pdInten[247] = 1912.842;
         pdMass[248] = 1147.58; pdInten[248] = 1749.472;
         pdMass[249] = 1148.947; pdInten[249] = 1643.691;
         pdMass[250] = 1153.078; pdInten[250] = 2976.508;
         pdMass[251] = 1161.584; pdInten[251] = 5949.839;
         pdMass[252] = 1162.08; pdInten[252] = 16522.71;
         pdMass[253] = 1173.617; pdInten[253] = 1938.755;
         pdMass[254] = 1184.676; pdInten[254] = 2236.742;
         pdMass[255] = 1201.691; pdInten[255] = 25661.4;
         pdMass[256] = 1202.509; pdInten[256] = 2386.245;
         pdMass[257] = 1202.691; pdInten[257] = 15303.19;
         pdMass[258] = 1203.498; pdInten[258] = 1817.653;
         pdMass[259] = 1203.695; pdInten[259] = 4767.838;
         pdMass[260] = 1212.65; pdInten[260] = 2624.244;
         pdMass[261] = 1217.136; pdInten[261] = 3047.852;
         pdMass[262] = 1217.633; pdInten[262] = 4777.614;
         pdMass[263] = 1218.14; pdInten[263] = 2467.369;
         pdMass[264] = 1220.684; pdInten[264] = 9110.043;
         pdMass[265] = 1221.687; pdInten[265] = 5432.289;
         pdMass[266] = 1222.694; pdInten[266] = 2129.724;
         pdMass[267] = 1225.648; pdInten[267] = 8666.314;
         pdMass[268] = 1226.15; pdInten[268] = 7839.705;
         pdMass[269] = 1226.655; pdInten[269] = 5541.688;
         pdMass[270] = 1227.16; pdInten[270] = 2147.979;
         pdMass[271] = 1234.168; pdInten[271] = 1890.851;
         pdMass[272] = 1234.661; pdInten[272] = 2574.847;
         pdMass[273] = 1237.735; pdInten[273] = 2202.117;
         pdMass[274] = 1282.189; pdInten[274] = 3352.796;
         pdMass[275] = 1314.772; pdInten[275] = 10168.11;
         pdMass[276] = 1315.776; pdInten[276] = 8736.916;
         pdMass[277] = 1316.781; pdInten[277] = 2953.505;
         pdMass[278] = 1327.737; pdInten[278] = 2484.055;
         pdMass[279] = 1339.233; pdInten[279] = 2412.756;
         pdMass[280] = 1339.73; pdInten[280] = 3766.912;
         pdMass[281] = 1344.693; pdInten[281] = 2324.504;
         pdMass[282] = 1349.727; pdInten[282] = 7682.241;
         pdMass[283] = 1350.735; pdInten[283] = 6557.733;
         pdMass[284] = 1371.746; pdInten[284] = 2379.279;
         pdMass[285] = 1380.255; pdInten[285] = 3805.205;
         pdMass[286] = 1380.75; pdInten[286] = 3603.762;
         pdMass[287] = 1381.251; pdInten[287] = 4166.542;
         pdMass[288] = 1389.751; pdInten[288] = 2386.491;
         pdMass[289] = 1404.245; pdInten[289] = 2583.889;
         pdMass[290] = 1404.734; pdInten[290] = 2539.731;
         pdMass[291] = 1405.237; pdInten[291] = 4398.562;
         pdMass[292] = 1406.751; pdInten[292] = 3148.7;
         pdMass[293] = 1407.748; pdInten[293] = 2697.42;
         pdMass[294] = 1413.747; pdInten[294] = 6355.567;
         pdMass[295] = 1414.247; pdInten[295] = 5174.805;
         pdMass[296] = 1427.863; pdInten[296] = 4909.611;
         pdMass[297] = 1428.864; pdInten[297] = 3539.385;
         pdMass[298] = 1435.282; pdInten[298] = 2241.931;
         pdMass[299] = 1450.275; pdInten[299] = 2286.532;
         pdMass[300] = 1457.784; pdInten[300] = 2831.026;
         pdMass[301] = 1464.772; pdInten[301] = 2219.887;
         pdMass[302] = 1490.744; pdInten[302] = 2804.959;
         pdMass[303] = 1510.898; pdInten[303] = 7767.534;
         pdMass[304] = 1511.896; pdInten[304] = 7274.059;
         pdMass[305] = 1512.903; pdInten[305] = 3783.454;
         pdMass[306] = 1533.288; pdInten[306] = 2207.758;
         pdMass[307] = 1565.3; pdInten[307] = 2280.441;
         pdMass[308] = 1589.849; pdInten[308] = 2677.061;
         pdMass[309] = 1623.98; pdInten[309] = 4380.018;
         pdMass[310] = 1624.982; pdInten[310] = 3728.6;
         pdMass[311] = 1650.839; pdInten[311] = 2835.313;
         pdMass[312] = 1695.775; pdInten[312] = 3621.624;
         pdMass[313] = 1696.776; pdInten[313] = 5628.164;
         pdMass[314] = 1853.055; pdInten[314] = 2090.654;
         pdMass[315] = 1854.053; pdInten[315] = 5277.071;
         pdMass[316] = 1946.926; pdInten[316] = 5950.179;
         pdMass[317] = 1968.095; pdInten[317] = 1870.645;


         // these are the return information from search
         sbyte[] szPeptide = new sbyte[512];
         sbyte[] szProtein = new sbyte[512];
         int iNumFragIons = 10;              // return 10 most intense matched b- and y-ions
         double[] pdYions = new double[iNumFragIons];
         double[] pdBions = new double[iNumFragIons];
         double[] pdScores = new double[5];   // 0=xcorr, 1=calc pep mass, 2=matched ions, 3=tot ions, 4=dCn

         // call Comet search here
         SearchMgr.DoSingleSpectrumSearch(iPrecursorCharge, dMZ, pdMass, pdInten, iNumPeaks,
            szPeptide, szProtein, pdYions, pdBions, iNumFragIons, pdScores);

         string peptide = Encoding.UTF8.GetString(szPeptide.Select(b=>(byte) b).ToArray());
         string protein = Encoding.UTF8.GetString(szProtein.Select(b => (byte)b).ToArray());
         int index = peptide.IndexOf('\0');
         if (index >= 0)
            peptide = peptide.Remove(index);
         index = protein.IndexOf('\0');
         if (index >= 0)
            protein = protein.Remove(index);

         double xcorr = pdScores[0];

         if (xcorr > 0)
         {
            Console.WriteLine("peptide: {0}\nprotein: {1}\nxcorr {2}\npepmass {3}\ndCn {4}\nions {5}/{6}",
               peptide, protein, pdScores[0], pdScores[1], pdScores[4], pdScores[2], (int)pdScores[3], (int)pdScores[4]);

            for (int i = 0; i < iNumFragIons; i++)
            {
               if (pdBions[i] > 0.0)
                  Console.WriteLine("matched b-ion {0}", pdBions[i]);
               else
                  break;
            }
            for (int i = 0; i < iNumFragIons; i++)
            {
               if (pdYions[i] > 0.0)
                  Console.WriteLine("matched y-ion {0}", pdYions[i]);
               else
                  break;
            }
         }
         else
         {
            Console.WriteLine("no match");
         }
      }
   }

   public class IntRange
   {
      public int Start { get; set; }
      public int End { get; set; }

      public IntRange()
      {
         Start = 0;
         End = 0;
      }

      public IntRange(int start, int end)
      {
         Start = start;
         End = end;
      }
   }

   public class DoubleRange
   {
      public double Start { get; set; }
      public double End { get; set; }

      public DoubleRange()
      {
         Start = 0;
         End = 0;
      }

      public DoubleRange(double start, double end)
      {
         Start = start;
         End = end;
      }
   }
}