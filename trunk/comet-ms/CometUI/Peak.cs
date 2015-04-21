using System.Collections.Generic;
using CometWrapper;

namespace CometUI
{
    public class Peak
    {
        public double Mz { get; set; }
        public double Intensity { get; set; }

        public Peak(double mz, double intensity)
        {
            Mz = mz;
            Intensity = intensity;
        }

        public Peak(Peak_T_Wrapper peakWrapper)
        {
            Mz = peakWrapper.get_mz();
            Intensity = peakWrapper.get_intensity();
        }
    }

    public class PeakIntensityComparer : IComparer<Peak>
    {
        public int Compare(Peak x, Peak y)
        {
            if (x.Intensity > y.Intensity)
            {
                return 1;
            }

            if (x.Intensity < y.Intensity)
            {
                return -1;
            }

            // They are equal
            return 0;
        }
    }

    public class PeakMzComparer : IComparer<Peak>
    {
        public int Compare(Peak x, Peak y)
        {
            if (x.Mz > y.Mz)
            {
                return 1;
            }

            if (x.Mz < y.Mz)
            {
                return -1;
            }

            // They are equal
            return 0;
        }
    }

    public class Peak_T_Wrapper_IntensityComparer : IComparer<Peak_T_Wrapper>
    {
        public int Compare(Peak_T_Wrapper x, Peak_T_Wrapper y)
        {
            if (x.get_intensity() > y.get_intensity())
            {
                return 1;
            }

            if (x.get_intensity() < y.get_intensity())
            {
                return -1;
            }

            // They are equal
            return 0;
        }
    }

    public class Peak_T_Wrapper_MzComparer : IComparer<Peak_T_Wrapper>
    {
        public int Compare(Peak_T_Wrapper x, Peak_T_Wrapper y)
        {
            if (x.get_mz() > y.get_mz())
            {
                return 1;
            }

            if (x.get_mz() < y.get_mz())
            {
                return -1;
            }

            // They are equal
            return 0;
        }
    }
}
