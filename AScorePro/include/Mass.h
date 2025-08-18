#pragma once

namespace AScoreProCpp {

    class Mass {
    public:
        enum class Units {
            PPM = 1,
            DALTON = 2
        };

        enum class IonSeries {
            A_NEUTRAL_LOSS = 1,
            B_NEUTRAL_LOSS = 2,
            Y_NEUTRAL_LOSS = 4,

            A_IONS = 8,
            B_IONS = 16,
            C_IONS = 32,

            D_IONS = 64,
            V_IONS = 128,
            W_IONS = 256,

            X_IONS = 512,
            Y_IONS = 1024,
            Z_IONS = 2048
        };

        // Proton mass = Hydrogen - e
        static constexpr double Proton = 1.007276466879000;

        // Neutral Hydrogen mass
        static constexpr double Hydrogen = 1.0078250321;

        static constexpr double Electron = 0.000548579867;

        static constexpr double Neutron = 1.00286864;

        static constexpr double Nitrogen = 14.0030740052;

        static constexpr double Carbon = 12.0000000;

        static constexpr double Oxygen = 15.9949146221;

        static constexpr double Water = 18.010564686;

        /**
         * Mass difference between isotopes of peptides when
         * assuming "Averagine" amino acid masses.
         */
        static constexpr double AveragineDifference = 1.00286864;
    };

    // Operator overloading for IonSeries enum to allow bitwise operations
    inline Mass::IonSeries operator|(Mass::IonSeries a, Mass::IonSeries b) {
        return static_cast<Mass::IonSeries>(static_cast<int>(a) | static_cast<int>(b));
    }

    inline Mass::IonSeries operator&(Mass::IonSeries a, Mass::IonSeries b) {
        return static_cast<Mass::IonSeries>(static_cast<int>(a) & static_cast<int>(b));
    }

    inline bool operator!(Mass::IonSeries a) {
        return static_cast<int>(a) == 0;
    }

    inline bool operator!=(Mass::IonSeries a, int b) {
        return static_cast<int>(a) != b;
    }

    inline bool operator==(Mass::IonSeries a, int b) {
        return static_cast<int>(a) == b;
    }

} // namespace AScoreProCpp