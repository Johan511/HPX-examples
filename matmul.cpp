#include <hpx/config.hpp>
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/util.hpp>

#include <chrono>
#include <iostream>
#include <random>
#include <vector>

std::mt19937 mt{};

// (mxn) * (nxk)
// matrix can be sub matrix
// la is width of actual matrix a
// ptr + la = ptr to cell vertically below it
void matmul(int m, int n, int k, double *a, int la, double *b, int lb,
            double *c, int lc) {
  int ii, jj, kk;
  for (ii = 0; ii < m; ii++) {
    for (jj = 0; jj < n; jj++) {
      double acc = 0.0;
      for (kk = 0; kk < k; kk++) {
        acc += a[ii + la * kk] * b[kk + lb * jj];
        // std::cout << acc << "+=" << a[ii + la * kk] << " * " << b[kk + lb *
        // jj]
        //           << '\n';
      }
      c[ii + lc * jj] = acc;
    }
  }
}

// generates matrix of (m*32)x(n*32)
double *generate_matrix(int m, int n) {
  double *matrix;
  int num_rows = m * 32;
  int num_cols = n * 32;

  matrix = (double *)malloc(sizeof(double) * num_rows * num_cols);

  for (int i = 0; i < num_rows * num_cols; i++) {
    matrix[i] = mt();
  }

  return matrix;
}

void multiply_matrix(double *A, double *B, double *C, int m, int n, int k) {
  std::vector<hpx::mutex> mutexes(m * k);
  std::vector<hpx::future<void>> futures(m * k * n);

  for (int ii = 0; ii < m; ii++)
    for (int kk = 0; kk < k; kk++)
      for (int jj = 0; jj < n; jj++) {
        futures[jj + kk * n + ii * k * n] = hpx::async(
            [&](int ii, int jj, int kk) {
              std::lock_guard<hpx::mutex> l(mutexes[kk + ii * k]);
              matmul(32, 32, 32, A, m, B, n, C, m);
            },
            ii, jj, kk);
      }

  for (auto &f : futures) {
    f.get();
  }
}

int hpx_main(hpx::program_options::variables_map &vm) {
  double *A; // m tiles x n tiles
  double *B; // n tiles x k tiles
  double *C; // m tiles x k tiles

  int m = vm["m"].as<int>();
  int n = vm["n"].as<int>();
  int k = vm["k"].as<int>();

  A = (double *)malloc(sizeof(double) * m * 32 * n * 32);
  B = (double *)malloc(sizeof(double) * n * 32 * k * 32);
  C = (double *)malloc(sizeof(double) * m * 32 * k * 32);

  if (!(A || B || C)) {
    std::cerr << "COULD NOT MALLOC" << std::endl;
    exit(1);
  }

  for (int i = 0; i < 20; i++) {

    std::for_each(A, A + m * 32 * n * 32, [](double &i) { i = rand(); });
    std::for_each(B, B + n * 32 * k * 32, [](double &i) { i = rand(); });
    std::for_each(C, C + m * 32 * k * 32, [](double &i) { i = rand(); });

    auto start = std::chrono::high_resolution_clock::now();
    multiply_matrix(A, B, C, m, n, k);
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end - start;
    std::cout << m << "," << duration.count() << std::endl;
  }

  return hpx::finalize();
}

int main(int argc, char *argv[]) {
  using namespace hpx::program_options;

  options_description desc_commandline;
  // clang-format off
  desc_commandline.add_options()("results",
                                 "print generated results (default: false)")
      ("m", value<int>()->default_value(10), "m")
      ("n", value<int>()->default_value(10), "n")
      ("k", value<int>()->default_value(10), "k");
  // clang-format on

  hpx::init_params init_args;
  init_args.desc_cmdline = desc_commandline;

  return hpx::init(argc, argv, init_args);
}
