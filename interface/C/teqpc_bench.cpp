#include <valarray>

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark_all.hpp>

#include <unordered_map>

// Prototypes of the functions exposed by the shared library
extern "C" int build_model(const char* j, char* uuid, char* errmsg, int errmsg_length);
extern "C" int free_model(const char* uid, char* errmsg, int errmsg_length);
extern "C" int get_Arxy(const char* uid, const int NT, const int ND, const double T, const double rho, const double* molefrac, const int Ncomp, double* val, char* errmsg, int errmsg_length);

TEST_CASE("teqpc profiling", "[teqpc]")
{
    const char* model = R"(  
        {
          "kind": "PCSAFT",
          "model": [
            {
              "BibTeXKey": "Gross-IECR-2001",
              "epsilon_over_k": 150.03,
              "m": 1.0,
              "name": "Methane",
              "sigma_Angstrom": 3.7039
            },
            {
              "BibTeXKey": "Gross-IECR-2001",
              "epsilon_over_k": 191.42,
              "m": 1.6069,
              "name": "Ethane",
              "sigma_Angstrom": 3.5206
            }
          ]
        }
    )";
    // Build the model
    char uid[40], errstr[200];
    int errcode = build_model(model, uid, errstr, 200);

    int NT = 0, ND = 1;
    double T = 300, rho = 0.5, out = -1;
    std::valarray<double> z = { 0.4, 0.6 };

    std::unordered_map<std::string, double> m = { {"afhgruelghrueoighfeklnieaogfyeogafuril", 1}, {"bgrheugiorehuglinfjlbhtuioyfr8gyriohguilfehvuioret7fregfilre", 4} };

    BENCHMARK("lookup") {
        return m["afhgruelghrueoighfeklnieaogfyeogafuril"];
    };
    BENCHMARK("build model") {
        int errcode = build_model(model, uid, errstr, 200);
        return uid;
    };
    BENCHMARK("call model") {
        double out = -1;
        int errcode2 = get_Arxy(uid, NT, ND, T, rho, &(z[0]), z.size(), &out, errstr, 200);
        return out;
    };
    
}