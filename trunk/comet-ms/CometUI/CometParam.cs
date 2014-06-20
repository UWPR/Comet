using System;

namespace CometUI
{
    public class CometParam
    {
        private CometParamType Type { get; set; }
        private String Value { get; set; }

        public CometParam(CometParamType paramType, ref String strValue)
        {
            Type = paramType;
            Value = strValue;
        }
    }

    public class TypedCometParam <T> : CometParam
    {
        public T Value { get; set; }

        public TypedCometParam(CometParamType paramType, String strValue, T value) : base(paramType, ref strValue)
        {
            // The field has the same type as the parameter.
            Value = value;
        }
    }

    public enum CometParamType
    {
        Unknown = 0,
        Int,
        Double,
        String,
        VarMods,
        DoubleRange,
        IntRange,
        EnzymeInfo
    }

    public class VarMods
    {
        public int BinaryMod { get; set; }
        public int MaxNumVarModAAPerMod { get; set; }
        public double VarModMass { get; set; }
        public String VarModChar { get; set; }

        public VarMods()
        {
            BinaryMod = 0;
            MaxNumVarModAAPerMod = 0;
            VarModMass = 0.0;
            VarModChar = String.Empty;
        }

        public VarMods(String varModChar, double varModMass, int binaryMod, int maxNumVarModPerMod)
        {
            VarModChar = varModChar;
            VarModMass = varModMass;
            BinaryMod = binaryMod;
            MaxNumVarModAAPerMod = maxNumVarModPerMod;
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
};
