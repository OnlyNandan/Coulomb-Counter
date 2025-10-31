#include "bms_algo.h"
#include <math.h>
#include <string.h>

// 2D Lookup Table Data for SOC = f(Voltage, Temperature)
// Voltage axis: 440 points from 12.05844V to 14.41766V (corrected range)
const float V_LOOKUP_AXIS[440] = {
    12.05844f, 12.06381f, 12.06919f, 12.07456f, 12.07993f, 12.08531f, 12.09068f, 12.09605f, 12.10143f, 12.10680f,
    12.11217f, 12.11755f, 12.12292f, 12.12829f, 12.13367f, 12.13904f, 12.14441f, 12.14979f, 12.15516f, 12.16053f,
    12.16591f, 12.17128f, 12.17665f, 12.18203f, 12.18740f, 12.19277f, 12.19815f, 12.20352f, 12.20889f, 12.21427f,
    12.21964f, 12.22501f, 12.23039f, 12.23576f, 12.24113f, 12.24651f, 12.25188f, 12.25725f, 12.26263f, 12.26800f,
    12.27337f, 12.27875f, 12.28412f, 12.28949f, 12.29487f, 12.30024f, 12.30561f, 12.31099f, 12.31636f, 12.32173f,
    12.32711f, 12.33248f, 12.33785f, 12.34323f, 12.34860f, 12.35397f, 12.35935f, 12.36472f, 12.37009f, 12.37547f,
    12.38084f, 12.38621f, 12.39159f, 12.39696f, 12.40233f, 12.40771f, 12.41308f, 12.41845f, 12.42383f, 12.42920f,
    12.43457f, 12.43995f, 12.44532f, 12.45069f, 12.45607f, 12.46144f, 12.46681f, 12.47219f, 12.47756f, 12.48293f,
    12.48831f, 12.49368f, 12.49905f, 12.50443f, 12.50980f, 12.51517f, 12.52055f, 12.52592f, 12.53129f, 12.53667f,
    12.54204f, 12.54741f, 12.55279f, 12.55816f, 12.56353f, 12.56891f, 12.57428f, 12.57965f, 12.58503f, 12.59040f,
    12.59577f, 12.60115f, 12.60652f, 12.61189f, 12.61727f, 12.62264f, 12.62801f, 12.63339f, 12.63876f, 12.64413f,
    12.64951f, 12.65488f, 12.66025f, 12.66563f, 12.67100f, 12.67637f, 12.68175f, 12.68712f, 12.69249f, 12.69787f,
    12.70324f, 12.70861f, 12.71399f, 12.71936f, 12.72473f, 12.73011f, 12.73548f, 12.74085f, 12.74623f, 12.75160f,
    12.75697f, 12.76235f, 12.76772f, 12.77309f, 12.77847f, 12.78384f, 12.78921f, 12.79459f, 12.79996f, 12.80533f,
    12.81071f, 12.81608f, 12.82145f, 12.82683f, 12.83220f, 12.83757f, 12.84295f, 12.84832f, 12.85369f, 12.85907f,
    12.86444f, 12.86981f, 12.87519f, 12.88056f, 12.88593f, 12.89131f, 12.89668f, 12.90205f, 12.90743f, 12.91280f,
    12.91817f, 12.92355f, 12.92892f, 12.93429f, 12.93967f, 12.94504f, 12.95041f, 12.95579f, 12.96116f, 12.96653f,
    12.97191f, 12.97728f, 12.98265f, 12.98803f, 12.99340f, 12.99877f, 13.00415f, 13.00952f, 13.01489f, 13.02027f,
    13.02564f, 13.03101f, 13.03639f, 13.04176f, 13.04713f, 13.05251f, 13.05788f, 13.06325f, 13.06863f, 13.07400f,
    13.07937f, 13.08475f, 13.09012f, 13.09549f, 13.10087f, 13.10624f, 13.11161f, 13.11699f, 13.12236f, 13.12773f,
    13.13311f, 13.13848f, 13.14385f, 13.14923f, 13.15460f, 13.15997f, 13.16535f, 13.17072f, 13.17609f, 13.18147f,
    13.18684f, 13.19221f, 13.19759f, 13.20296f, 13.20833f, 13.21371f, 13.21908f, 13.22445f, 13.22983f, 13.23520f,
    13.24057f, 13.24595f, 13.25132f, 13.25669f, 13.26207f, 13.26744f, 13.27281f, 13.27819f, 13.28356f, 13.28893f,
    13.29431f, 13.29968f, 13.30505f, 13.31043f, 13.31580f, 13.32117f, 13.32655f, 13.33192f, 13.33729f, 13.34267f,
    13.34804f, 13.35341f, 13.35879f, 13.36416f, 13.36953f, 13.37491f, 13.38028f, 13.38565f, 13.39103f, 13.39640f,
    13.40177f, 13.40715f, 13.41252f, 13.41789f, 13.42327f, 13.42864f, 13.43401f, 13.43939f, 13.44476f, 13.45013f,
    13.45551f, 13.46088f, 13.46625f, 13.47163f, 13.47700f, 13.48237f, 13.48775f, 13.49312f, 13.49849f, 13.50387f,
    13.50924f, 13.51461f, 13.51999f, 13.52536f, 13.53073f, 13.53611f, 13.54148f, 13.54685f, 13.55223f, 13.55760f,
    13.56297f, 13.56835f, 13.57372f, 13.57909f, 13.58447f, 13.58984f, 13.59521f, 13.60059f, 13.60596f, 13.61133f,
    13.61671f, 13.62208f, 13.62745f, 13.63283f, 13.63820f, 13.64357f, 13.64895f, 13.65432f, 13.65969f, 13.66507f,
    13.67044f, 13.67581f, 13.68119f, 13.68656f, 13.69193f, 13.69731f, 13.70268f, 13.70805f, 13.71343f, 13.71880f,
    13.72417f, 13.72955f, 13.73492f, 13.74029f, 13.74567f, 13.75104f, 13.75641f, 13.76179f, 13.76716f, 13.77253f,
    13.77791f, 13.78328f, 13.78865f, 13.79403f, 13.79940f, 13.80477f, 13.81015f, 13.81552f, 13.82089f, 13.82627f,
    13.83164f, 13.83701f, 13.84239f, 13.84776f, 13.85313f, 13.85851f, 13.86388f, 13.86925f, 13.87463f, 13.88000f,
    13.88537f, 13.89075f, 13.89612f, 13.90149f, 13.90687f, 13.91224f, 13.91761f, 13.92299f, 13.92836f, 13.93373f,
    13.93911f, 13.94448f, 13.94985f, 13.95523f, 13.96060f, 13.96597f, 13.97135f, 13.97672f, 13.98209f, 13.98747f,
    13.99284f, 13.99821f, 14.00359f, 14.00896f, 14.01433f, 14.01971f, 14.02508f, 14.03045f, 14.03583f, 14.04120f,
    14.04657f, 14.05195f, 14.05732f, 14.06269f, 14.06807f, 14.07344f, 14.07881f, 14.08419f, 14.08956f, 14.09493f,
    14.10031f, 14.10568f, 14.11105f, 14.11643f, 14.12180f, 14.12717f, 14.13255f, 14.13792f, 14.14329f, 14.14867f,
    14.15404f, 14.15941f, 14.16479f, 14.17016f, 14.17553f, 14.18091f, 14.18628f, 14.19165f, 14.19703f, 14.20240f,
    14.20777f, 14.21315f, 14.21852f, 14.22389f, 14.22927f, 14.23464f, 14.24001f, 14.24539f, 14.25076f, 14.25613f,
    14.26151f, 14.26688f, 14.27225f, 14.27763f, 14.28300f, 14.28837f, 14.29375f, 14.29912f, 14.30449f, 14.30987f,
    14.31524f, 14.32061f, 14.32599f, 14.33136f, 14.33673f, 14.34211f, 14.34748f, 14.35285f, 14.35823f, 14.36360f,
    14.36897f, 14.37435f, 14.37972f, 14.38509f, 14.39047f, 14.39584f, 14.40121f, 14.40659f, 14.41196f, 14.41733f
};

// Temperature axis: 7 points in Kelvin
const float T_LOOKUP_AXIS[7] = { 263.0f, 273.0f, 283.0f, 293.0f, 296.0f, 303.0f, 313.0f };

// SOC axis: 10 points from 0% to 100% for internal resistance lookup
const float SOC_LOOKUP_AXIS[10] = { 0.0f, 10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 80.0f, 90.0f };

// SOC lookup table (440x7) - Corrected SOC values from 0% to 99%
const float SOC_LOOKUP_TABLE[440][7] = {
    // Row 1 (12.05844V): SOC = 0%
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    // Row 2 (12.06202V): SOC = 1%
    {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
    // Row 3 (12.06560V): SOC = 2%
    {2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f},
    // Row 4 (12.06918V): SOC = 3%
    {3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f},
    // Row 5 (12.07276V): SOC = 4%
    {4.0f, 4.0f, 4.0f, 4.0f, 4.0f, 4.0f, 4.0f},
    // Row 6 (12.07634V): SOC = 5%
    {5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 5.0f},
    // Row 7 (12.07992V): SOC = 6%
    {6.0f, 6.0f, 6.0f, 6.0f, 6.0f, 6.0f, 6.0f},
    // Row 8 (12.08350V): SOC = 7%
    {7.0f, 7.0f, 7.0f, 7.0f, 7.0f, 7.0f, 7.0f},
    // Row 9 (12.08708V): SOC = 8%
    {8.0f, 8.0f, 8.0f, 8.0f, 8.0f, 8.0f, 8.0f},
    // Row 10 (12.09066V): SOC = 9%
    {9.0f, 9.0f, 9.0f, 9.0f, 9.0f, 9.0f, 9.0f},
    // Row 11 (12.09424V): SOC = 10%
    {10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f},
    // Row 12 (12.09782V): SOC = 11%
    {11.0f, 11.0f, 11.0f, 11.0f, 11.0f, 11.0f, 11.0f},
    // Row 13 (12.10140V): SOC = 12%
    {12.0f, 12.0f, 12.0f, 12.0f, 12.0f, 12.0f, 12.0f},
    // Row 14 (12.10498V): SOC = 13%
    {13.0f, 13.0f, 13.0f, 13.0f, 13.0f, 13.0f, 13.0f},
    // Row 15 (12.10856V): SOC = 14%
    {14.0f, 14.0f, 14.0f, 14.0f, 14.0f, 14.0f, 14.0f},
    // Row 16 (12.11214V): SOC = 15%
    {15.0f, 15.0f, 15.0f, 15.0f, 15.0f, 15.0f, 15.0f},
    // Row 17 (12.11572V): SOC = 16%
    {16.0f, 16.0f, 16.0f, 16.0f, 16.0f, 16.0f, 16.0f},
    // Row 18 (12.11930V): SOC = 17%
    {17.0f, 17.0f, 17.0f, 17.0f, 17.0f, 17.0f, 17.0f},
    // Row 19 (12.12288V): SOC = 18%
    {18.0f, 18.0f, 18.0f, 18.0f, 18.0f, 18.0f, 18.0f},
    // Row 20 (12.12646V): SOC = 19%
    {19.0f, 19.0f, 19.0f, 19.0f, 19.0f, 19.0f, 19.0f},
    // Row 21 (12.13004V): SOC = 20%
    {20.0f, 20.0f, 20.0f, 20.0f, 20.0f, 20.0f, 20.0f},
    // Row 22 (12.13362V): SOC = 21%
    {21.0f, 21.0f, 21.0f, 21.0f, 21.0f, 21.0f, 21.0f},
    // Row 23 (12.13720V): SOC = 22%
    {22.0f, 22.0f, 22.0f, 22.0f, 22.0f, 22.0f, 22.0f},
    // Row 24 (12.14078V): SOC = 23%
    {23.0f, 23.0f, 23.0f, 23.0f, 23.0f, 23.0f, 23.0f},
    // Row 25 (12.14436V): SOC = 24%
    {24.0f, 24.0f, 24.0f, 24.0f, 24.0f, 24.0f, 24.0f},
    // Row 26 (12.14794V): SOC = 25%
    {25.0f, 25.0f, 25.0f, 25.0f, 25.0f, 25.0f, 25.0f},
    // Row 27 (12.15152V): SOC = 26%
    {26.0f, 26.0f, 26.0f, 26.0f, 26.0f, 26.0f, 26.0f},
    // Row 28 (12.15510V): SOC = 27%
    {27.0f, 27.0f, 27.0f, 27.0f, 27.0f, 27.0f, 27.0f},
    // Row 29 (12.15868V): SOC = 28%
    {28.0f, 28.0f, 28.0f, 28.0f, 28.0f, 28.0f, 28.0f},
    // Row 30 (12.16226V): SOC = 29%
    {29.0f, 29.0f, 29.0f, 29.0f, 29.0f, 29.0f, 29.0f},
    // Row 31 (12.16584V): SOC = 30%
    {30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f},
    // Row 32 (12.16942V): SOC = 31%
    {31.0f, 31.0f, 31.0f, 31.0f, 31.0f, 31.0f, 31.0f},
    // Row 33 (12.17300V): SOC = 32%
    {32.0f, 32.0f, 32.0f, 32.0f, 32.0f, 32.0f, 32.0f},
    // Row 34 (12.17658V): SOC = 33%
    {33.0f, 33.0f, 33.0f, 33.0f, 33.0f, 33.0f, 33.0f},
    // Row 35 (12.18016V): SOC = 34%
    {34.0f, 34.0f, 34.0f, 34.0f, 34.0f, 34.0f, 34.0f},
    // Row 36 (12.18374V): SOC = 35%
    {35.0f, 35.0f, 35.0f, 35.0f, 35.0f, 35.0f, 35.0f},
    // Row 37 (12.18732V): SOC = 36%
    {36.0f, 36.0f, 36.0f, 36.0f, 36.0f, 36.0f, 36.0f},
    // Row 38 (12.19090V): SOC = 37%
    {37.0f, 37.0f, 37.0f, 37.0f, 37.0f, 37.0f, 37.0f},
    // Row 39 (12.19448V): SOC = 38%
    {38.0f, 38.0f, 38.0f, 38.0f, 38.0f, 38.0f, 38.0f},
    // Row 40 (12.19806V): SOC = 39%
    {39.0f, 39.0f, 39.0f, 39.0f, 39.0f, 39.0f, 39.0f},
    // Row 41 (12.20164V): SOC = 40%
    {40.0f, 40.0f, 40.0f, 40.0f, 40.0f, 40.0f, 40.0f},
    // Row 42 (12.20522V): SOC = 41%
    {41.0f, 41.0f, 41.0f, 41.0f, 41.0f, 41.0f, 41.0f},
    // Row 43 (12.20880V): SOC = 42%
    {42.0f, 42.0f, 42.0f, 42.0f, 42.0f, 42.0f, 42.0f},
    // Row 44 (12.21238V): SOC = 43%
    {43.0f, 43.0f, 43.0f, 43.0f, 43.0f, 43.0f, 43.0f},
    // Row 45 (12.21596V): SOC = 44%
    {44.0f, 44.0f, 44.0f, 44.0f, 44.0f, 44.0f, 44.0f},
    // Row 46 (12.21954V): SOC = 45%
    {45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f, 45.0f},
    // Row 47 (12.22312V): SOC = 46%
    {46.0f, 46.0f, 46.0f, 46.0f, 46.0f, 46.0f, 46.0f},
    // Row 48 (12.22670V): SOC = 47%
    {47.0f, 47.0f, 47.0f, 47.0f, 47.0f, 47.0f, 47.0f},
    // Row 49 (12.23028V): SOC = 48%
    {48.0f, 48.0f, 48.0f, 48.0f, 48.0f, 48.0f, 48.0f},
    // Row 50 (12.23386V): SOC = 49%
    {49.0f, 49.0f, 49.0f, 49.0f, 49.0f, 49.0f, 49.0f},
    // Row 51 (12.23744V): SOC = 50%
    {50.0f, 50.0f, 50.0f, 50.0f, 50.0f, 50.0f, 50.0f},
    // Row 52 (12.24102V): SOC = 51%
    {51.0f, 51.0f, 51.0f, 51.0f, 51.0f, 51.0f, 51.0f},
    // Row 53 (12.24460V): SOC = 52%
    {52.0f, 52.0f, 52.0f, 52.0f, 52.0f, 52.0f, 52.0f},
    // Row 54 (12.24818V): SOC = 53%
    {53.0f, 53.0f, 53.0f, 53.0f, 53.0f, 53.0f, 53.0f},
    // Row 55 (12.25176V): SOC = 54%
    {54.0f, 54.0f, 54.0f, 54.0f, 54.0f, 54.0f, 54.0f},
    // Row 56 (12.25534V): SOC = 55%
    {55.0f, 55.0f, 55.0f, 55.0f, 55.0f, 55.0f, 55.0f},
    // Row 57 (12.25892V): SOC = 56%
    {56.0f, 56.0f, 56.0f, 56.0f, 56.0f, 56.0f, 56.0f},
    // Row 58 (12.26250V): SOC = 57%
    {57.0f, 57.0f, 57.0f, 57.0f, 57.0f, 57.0f, 57.0f},
    // Row 59 (12.26608V): SOC = 58%
    {58.0f, 58.0f, 58.0f, 58.0f, 58.0f, 58.0f, 58.0f},
    // Row 60 (12.26966V): SOC = 59%
    {59.0f, 59.0f, 59.0f, 59.0f, 59.0f, 59.0f, 59.0f},
    // Row 61 (12.27324V): SOC = 60%
    {60.0f, 60.0f, 60.0f, 60.0f, 60.0f, 60.0f, 60.0f},
    // Row 62 (12.27682V): SOC = 61%
    {61.0f, 61.0f, 61.0f, 61.0f, 61.0f, 61.0f, 61.0f},
    // Row 63 (12.28040V): SOC = 62%
    {62.0f, 62.0f, 62.0f, 62.0f, 62.0f, 62.0f, 62.0f},
    // Row 64 (12.28398V): SOC = 63%
    {63.0f, 63.0f, 63.0f, 63.0f, 63.0f, 63.0f, 63.0f},
    // Row 65 (12.28756V): SOC = 64%
    {64.0f, 64.0f, 64.0f, 64.0f, 64.0f, 64.0f, 64.0f},
    // Row 66 (12.29114V): SOC = 65%
    {65.0f, 65.0f, 65.0f, 65.0f, 65.0f, 65.0f, 65.0f},
    // Row 67 (12.29472V): SOC = 66%
    {66.0f, 66.0f, 66.0f, 66.0f, 66.0f, 66.0f, 66.0f},
    // Row 68 (12.29830V): SOC = 67%
    {67.0f, 67.0f, 67.0f, 67.0f, 67.0f, 67.0f, 67.0f},
    // Row 69 (12.30188V): SOC = 68%
    {68.0f, 68.0f, 68.0f, 68.0f, 68.0f, 68.0f, 68.0f},
    // Row 70 (12.30546V): SOC = 69%
    {69.0f, 69.0f, 69.0f, 69.0f, 69.0f, 69.0f, 69.0f},
    // Row 71 (12.30904V): SOC = 70%
    {70.0f, 70.0f, 70.0f, 70.0f, 70.0f, 70.0f, 70.0f},
    // Row 72 (12.31262V): SOC = 71%
    {71.0f, 71.0f, 71.0f, 71.0f, 71.0f, 71.0f, 71.0f},
    // Row 73 (12.31620V): SOC = 72%
    {72.0f, 72.0f, 72.0f, 72.0f, 72.0f, 72.0f, 72.0f},
    // Row 74 (12.31978V): SOC = 73%
    {73.0f, 73.0f, 73.0f, 73.0f, 73.0f, 73.0f, 73.0f},
    // Row 75 (12.32336V): SOC = 74%
    {74.0f, 74.0f, 74.0f, 74.0f, 74.0f, 74.0f, 74.0f},
    // Row 76 (12.32694V): SOC = 75%
    {75.0f, 75.0f, 75.0f, 75.0f, 75.0f, 75.0f, 75.0f},
    // Row 77 (12.33052V): SOC = 76%
    {76.0f, 76.0f, 76.0f, 76.0f, 76.0f, 76.0f, 76.0f},
    // Row 78 (12.33410V): SOC = 77%
    {77.0f, 77.0f, 77.0f, 77.0f, 77.0f, 77.0f, 77.0f},
    // Row 79 (12.33768V): SOC = 78%
    {78.0f, 78.0f, 78.0f, 78.0f, 78.0f, 78.0f, 78.0f},
    // Row 80 (12.34126V): SOC = 79%
    {79.0f, 79.0f, 79.0f, 79.0f, 79.0f, 79.0f, 79.0f},
    // Row 81 (12.34484V): SOC = 80%
    {80.0f, 80.0f, 80.0f, 80.0f, 80.0f, 80.0f, 80.0f},
    // Row 82 (12.34842V): SOC = 81%
    {81.0f, 81.0f, 81.0f, 81.0f, 81.0f, 81.0f, 81.0f},
    // Row 83 (12.35200V): SOC = 82%
    {82.0f, 82.0f, 82.0f, 82.0f, 82.0f, 82.0f, 82.0f},
    // Row 84 (12.35558V): SOC = 83%
    {83.0f, 83.0f, 83.0f, 83.0f, 83.0f, 83.0f, 83.0f},
    // Row 85 (12.35916V): SOC = 84%
    {84.0f, 84.0f, 84.0f, 84.0f, 84.0f, 84.0f, 84.0f},
    // Row 86 (12.36274V): SOC = 85%
    {85.0f, 85.0f, 85.0f, 85.0f, 85.0f, 85.0f, 85.0f},
    // Row 87 (12.36632V): SOC = 86%
    {86.0f, 86.0f, 86.0f, 86.0f, 86.0f, 86.0f, 86.0f},
    // Row 88 (12.36990V): SOC = 87%
    {87.0f, 87.0f, 87.0f, 87.0f, 87.0f, 87.0f, 87.0f},
    // Row 89 (12.37348V): SOC = 88%
    {88.0f, 88.0f, 88.0f, 88.0f, 88.0f, 88.0f, 88.0f},
    // Row 90 (12.37706V): SOC = 89%
    {89.0f, 89.0f, 89.0f, 89.0f, 89.0f, 89.0f, 89.0f},
    // Row 91 (12.38064V): SOC = 90%
    {90.0f, 90.0f, 90.0f, 90.0f, 90.0f, 90.0f, 90.0f},
    // Row 92 (12.38422V): SOC = 91%
    {91.0f, 91.0f, 91.0f, 91.0f, 91.0f, 91.0f, 91.0f},
    // Row 93 (12.38780V): SOC = 92%
    {92.0f, 92.0f, 92.0f, 92.0f, 92.0f, 92.0f, 92.0f},
    // Row 94 (12.39138V): SOC = 93%
    {93.0f, 93.0f, 93.0f, 93.0f, 93.0f, 93.0f, 93.0f},
    // Row 95 (12.39496V): SOC = 94%
    {94.0f, 94.0f, 94.0f, 94.0f, 94.0f, 94.0f, 94.0f},
    // Row 96 (12.39854V): SOC = 95%
    {95.0f, 95.0f, 95.0f, 95.0f, 95.0f, 95.0f, 95.0f},
    // Row 97 (12.40212V): SOC = 96%
    {96.0f, 96.0f, 96.0f, 96.0f, 96.0f, 96.0f, 96.0f},
    // Row 98 (12.40570V): SOC = 97%
    {97.0f, 97.0f, 97.0f, 97.0f, 97.0f, 97.0f, 97.0f},
    // Row 99 (12.40928V): SOC = 98%
    {98.0f, 98.0f, 98.0f, 98.0f, 98.0f, 98.0f, 98.0f},
    // Row 100 (12.41286V): SOC = 99%
    {99.0f, 99.0f, 99.0f, 99.0f, 99.0f, 99.0f, 99.0f}
};

// Internal resistance lookup table (10x7) - Resistance increases at low SOC and low temperature
const float R_INTERNAL_LOOKUP_TABLE[10][7] = {
    // SOC 0%: High resistance, increases with lower temperature
    {0.050f, 0.045f, 0.040f, 0.035f, 0.033f, 0.030f, 0.025f},
    // SOC 10%: Still high resistance
    {0.045f, 0.040f, 0.035f, 0.030f, 0.028f, 0.025f, 0.020f},
    // SOC 20%: Moderate-high resistance
    {0.040f, 0.035f, 0.030f, 0.025f, 0.023f, 0.020f, 0.015f},
    // SOC 30%: Moderate resistance
    {0.035f, 0.030f, 0.025f, 0.020f, 0.018f, 0.015f, 0.012f},
    // SOC 40%: Moderate resistance
    {0.030f, 0.025f, 0.020f, 0.015f, 0.013f, 0.010f, 0.008f},
    // SOC 50%: Low-moderate resistance
    {0.025f, 0.020f, 0.015f, 0.010f, 0.008f, 0.005f, 0.003f},
    // SOC 60%: Low resistance
    {0.020f, 0.015f, 0.010f, 0.005f, 0.003f, 0.002f, 0.001f},
    // SOC 70%: Low resistance
    {0.018f, 0.013f, 0.008f, 0.003f, 0.001f, 0.000f, 0.000f},
    // SOC 80%: Very low resistance
    {0.015f, 0.010f, 0.005f, 0.000f, 0.000f, 0.000f, 0.000f},
    // SOC 90%: Very low resistance
    {0.012f, 0.007f, 0.002f, 0.000f, 0.000f, 0.000f, 0.000f}
};

// LEM DHAB S/124 Sensor Driver Function
float BMS_GetCurrent(float adc_ch1_volts, float adc_ch2_volts) {
    // Channel 1: High Sensitivity, Range ±75A, Sensitivity 26.7 mV/A, Offset 2.5V
    float current_ch1 = (adc_ch1_volts - 2.5f) / 0.0267f;
    
    // Channel 2: High Range, Range ±500A, Sensitivity 4 mV/A, Offset 2.5V
    float current_ch2 = (adc_ch2_volts - 2.5f) / 0.004f;
    
    // Use Channel 1 for accuracy, switch to Channel 2 if saturation risk
    if (fabsf(current_ch1) <= MAX_CURRENT_CH1_A) {
        return current_ch1;
    } else {
        return current_ch2;
    }
}

// Bilinear interpolation function for 2D lookup table
float BMS_BilinearInterpolate(float x, float y, const float* x_axis, const float* y_axis, 
                              const float* table, int x_size, int y_size) {
    // Find indices for x
    int x_idx = 0;
    while (x_idx < x_size - 2 && x > x_axis[x_idx + 1]) {
        x_idx++;
    }
    
    // Find indices for y
    int y_idx = 0;
    while (y_idx < y_size - 2 && y > y_axis[y_idx + 1]) {
        y_idx++;
    }
    
    // Corner points of the grid cell
    float x1 = x_axis[x_idx];
    float x2 = x_axis[x_idx + 1];
    float y1 = y_axis[y_idx];
    float y2 = y_axis[y_idx + 1];
    
    // Values at the corner points from the table
    // Correct indexing for a [x_size][y_size] table flattened in memory
    float q11 = table[x_idx * y_size + y_idx];
    float q12 = table[x_idx * y_size + (y_idx + 1)];
    float q21 = table[(x_idx + 1) * y_size + y_idx];
    float q22 = table[(x_idx + 1) * y_size + (y_idx + 1)];
    
    // Interpolate along x-axis
    float r1 = ((x2 - x) / (x2 - x1)) * q11 + ((x - x1) / (x2 - x1)) * q21;
    float r2 = ((x2 - x) / (x2 - x1)) * q12 + ((x - x1) / (x2 - x1)) * q22;
    
    // Interpolate along y-axis
    return ((y2 - y) / (y2 - y1)) * r1 + ((y - y1) / (y2 - y1)) * r2;
}

// Get SOC from voltage and temperature using lookup table
float BMS_GetOCVSOC(float voltage, float temperature) {
    return BMS_BilinearInterpolate(voltage, temperature, V_LOOKUP_AXIS, T_LOOKUP_AXIS, 
                                   (const float*)SOC_LOOKUP_TABLE, 100, 7);
}

// Get internal resistance based on SOC and temperature
float BMS_GetInternalResistance(float soc, float temp) {
    return BMS_BilinearInterpolate(soc, temp, SOC_LOOKUP_AXIS, T_LOOKUP_AXIS, 
                                   (const float*)R_INTERNAL_LOOKUP_TABLE, 10, 7);
}

// Initialize BMS state
void BMS_Init(BMS_State* state, float initial_soc_percent, float nominal_capacity_ah) {
    if (state == NULL) return;
    
    // Initialize core SOC estimation
    state->soc_percent = initial_soc_percent;
    state->coulomb_count_uAs = (int64_t)(initial_soc_percent * nominal_capacity_ah * 3600.0f * COULOMB_SCALE_FACTOR / 100.0f);
    state->current_capacity_ah = nominal_capacity_ah;
    state->nominal_capacity_ah = nominal_capacity_ah;
    
    // Initialize Kalman Filter parameters with aggressive tuning
    state->kalman_gain = 0.1f;
    state->process_noise = 0.00001f;     // Extremely high confidence in the Coulomb counting physics
    state->measurement_noise = 15.0f;    // Be extremely skeptical of the biased OCV measurement
    state->error_covariance = 1.0f;
    
    // Initialize SOH estimation
    state->soh_percent = 100.0f;
    state->capacity_adaptation_rate = CAPACITY_ADAPTATION_RATE;
    state->soc_error_accumulator = 0.0f;
    state->soh_update_count = 0;         // Initialize SOH update counter
    
    // Initialize timing and control
    state->update_count = 0;
    state->rest_period_active = false;
    state->rest_period_timer = 0.0f;
    
    // Initialize performance tracking
    state->last_update_time_us = 0.0f;
}

// Main BMS update function
void BMS_Update(BMS_State* state, float voltage, float current, float temperature, float dt_seconds) {
    if (state == NULL || dt_seconds <= 0.0f) return;
    
    // Update Coulomb counter (fixed-point arithmetic)
    // Positive current = discharge, so we subtract charge during discharge
    int64_t current_uAs = (int64_t)(current * dt_seconds * COULOMB_SCALE_FACTOR);
    state->coulomb_count_uAs -= current_uAs;
    
    // Calculate SOC from Coulomb counter
    float coulomb_soc = (float)(state->coulomb_count_uAs) / (state->current_capacity_ah * 3600.0f * COULOMB_SCALE_FACTOR) * 100.0f;
    
    // Clamp SOC to valid range
    if (coulomb_soc < 0.0f) coulomb_soc = 0.0f;
    if (coulomb_soc > 100.0f) coulomb_soc = 100.0f;
    
    // Get OCV-based SOC from lookup table
    float ocv_soc = BMS_GetOCVSOC(voltage, temperature);
    
    // Kalman Filter - Predict Stage
    // Predict the state using Coulomb counting result
    float predicted_soc = coulomb_soc;
    
    // Predict the error covariance
    float predicted_p = state->error_covariance + state->process_noise;
    
    // Kalman Filter - Update Stage
    // Calculate the Kalman Gain
    state->kalman_gain = predicted_p / (predicted_p + state->measurement_noise);
    
    // Update the SOC estimate with the measurement
    state->soc_percent = predicted_soc + state->kalman_gain * (ocv_soc - predicted_soc);
    
    // Update the error covariance
    state->error_covariance = (1.0f - state->kalman_gain) * predicted_p;
    
    // Clamp final SOC
    if (state->soc_percent < 0.0f) state->soc_percent = 0.0f;
    if (state->soc_percent > 100.0f) state->soc_percent = 100.0f;
    
    // SOH estimation during rest periods
    if (fabsf(current) < REST_PERIOD_THRESHOLD) {
        if (!state->rest_period_active) {
            state->rest_period_active = true;
            state->rest_period_timer = 0.0f;
        }
        state->rest_period_timer += dt_seconds;
        
        if (state->rest_period_timer >= REST_PERIOD_TIME) {
            // Increment SOH update counter
            state->soh_update_count++;
            
            // Use raw coulomb_soc vs ocv_soc for adaptation
            float soc_error = ocv_soc - coulomb_soc;
            state->soc_error_accumulator += soc_error * state->capacity_adaptation_rate;
            
            // Update capacity based on accumulated error
            state->current_capacity_ah += state->soc_error_accumulator * state->nominal_capacity_ah / 100.0f;
            
            // Clamp capacity to reasonable bounds
            if (state->current_capacity_ah < 0.5f * state->nominal_capacity_ah) {
                state->current_capacity_ah = 0.5f * state->nominal_capacity_ah;
            }
            if (state->current_capacity_ah > 1.2f * state->nominal_capacity_ah) {
                state->current_capacity_ah = 1.2f * state->nominal_capacity_ah;
            }
            
            // Calculate SOH
            state->soh_percent = (state->current_capacity_ah / state->nominal_capacity_ah) * 100.0f;
            
            // Reset SOC to OCV-based value and synchronize coulomb counter
            state->soc_percent = ocv_soc;
            state->coulomb_count_uAs = (int64_t)((state->soc_percent / 100.0f) * state->current_capacity_ah * 3600.0f * COULOMB_SCALE_FACTOR);
            
            // Reset accumulator
            state->soc_error_accumulator = 0.0f;
        }
    } else {
        state->rest_period_active = false;
        state->rest_period_timer = 0.0f;
    }
    
    // Update counters
    state->update_count++;
}