#include <vector>
#include <iostream>
#include <cmath>
#include <random>
#include <fstream>

#include <range_mi/barely_distorted.hpp>

// Define constants
double integration_step = 0.00001;
double vacancy_scaling = 10;
double dtheta = 0.1;
double noise_l = 99999999;
unsigned int num_cells = 100;
unsigned int num_dimensions = 5;

void numerical_pdf(
    const unsigned int * const line,
    const double * const vacancy,
    const double * const width,
    unsigned int num_cells,
    double * const pdf,
    unsigned int & pdf_size) {

  unsigned int i = 0;
  double r = 0;
  double width_sum = 0;
  double pdf_decay = 1;
  pdf_size = 0;
  while (i < num_cells) {
    unsigned int j = line[i];

    // Compute the pdf
    pdf[pdf_size++] = 
      pdf_decay * (
          -std::pow(vacancy[j], r - width_sum) *
          std::log(vacancy[j]));

    // Make a step
    r += integration_step;
    if (r - width_sum > width[i]) {
      // Cell completed,
      // move to the next cell!
      pdf_decay *= std::pow(vacancy[j], width[i]);
      width_sum += width[i];
      i++;
    }
  }
}

double numerical_mi(
    const double * const pdf,
    unsigned pdf_size,
    unsigned int dimension) {

  double h_Z = 0;
  double h_Z_given_M = 0;
  for (unsigned int i = 0; i < pdf_size; i++) {
    double r = i * integration_step;

    h_Z +=
      pdf[i] *
      std::pow(r, dimension - 1) *
      -std::log(pdf[i]) *
      integration_step *
      dtheta;

    h_Z_given_M +=
      (1 - std::log(noise_l)) *
      pdf[i] *
      std::pow(r, dimension - 1) *
      integration_step *
      dtheta;
  }

  return h_Z - h_Z_given_M;
}

// Initialize random generator
std::random_device random_device;
std::mt19937 gen(random_device());
std::uniform_real_distribution<double> dist(0.,1.);

// ... and a way to make random vectors
void random_p(std::vector<double> & p) {
  for (size_t i = 0; i < p.size(); i++) {
    p[i] = dist(gen);
  }
}

int main() {
  // Initialize the inputs and outputs
  std::vector<double> vacancy(num_cells);
  std::vector<double> p_not_measured(num_cells, 1);
  std::vector<double> width(num_cells);
  // Randomize then
  random_p(vacancy);
  random_p(width);

  // Scale vacancy to make everything generally lower
  for (unsigned int i = 0; i < num_cells; i++) {
    vacancy[i] = std::pow(vacancy[i], vacancy_scaling);
  }

  // Make a line
  std::vector<unsigned int> line(num_cells);
  std::iota(line.begin(), line.end(), 0);

  std::cout << "Computing the barely distorted pdf" << std::endl;

  // Compute the pdf
  unsigned int pdf_size;
  std::vector<double> pdf(num_cells/integration_step);
  numerical_pdf(
      line.data(),
      vacancy.data(),
      width.data(),
      num_cells,
      pdf.data(),
      pdf_size);

  std::cout << "Computing the barely distorted mutual information numerically..." << std::endl;

  std::vector<double> numerical_mi_(num_dimensions);
  for (unsigned int i = 0; i < num_dimensions; i++) {
    numerical_mi_[i] = numerical_mi(pdf.data(), pdf_size, i + 1);
  }

  std::cout << "Computing the barely distorted mutual information exactly..." << std::endl;

  std::vector<double> exact_mi(num_dimensions, 0);
  std::vector<double> mi(num_cells);
  for (unsigned int i = 0; i < num_dimensions; i++) {
    // Clear the output
    std::fill(mi.begin(), mi.end(), 0);

    range_mi::barely_distorted::line(
        line.data(),
        vacancy.data(),
        p_not_measured.data(),
        width.data(),
        num_cells,
        dtheta,
        noise_l,
        i + 1,
        mi.data());

    exact_mi[i] = mi[0];
  }

  std::cout << std::endl;
  for (unsigned int i = 0; i < num_dimensions; i++) {
    std::cout << "d" << i + 1 << ": " << numerical_mi_[i] << ", " << exact_mi[i] << std::endl;
  }
}