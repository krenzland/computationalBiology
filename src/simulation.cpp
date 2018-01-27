#include <algorithm>
#include <iostream>
#include <fstream>
#include <random>
#include <vector>
#include <cassert>
#include <tuple>

template<typename RandomGenerator>
int getSlidingLength(int avgSlidingLen, RandomGenerator& gen) {
  if (avgSlidingLen == 0) {
    return 0;
  }
  // Compute parameters of distribution.
  const double a = avgSlidingLen + avgSlidingLen * avgSlidingLen;
  const double b = 1 - (a - avgSlidingLen)/a;
  auto slidingDistribution = std::negative_binomial_distribution<int>(1, b);

  return slidingDistribution(gen);
}

template<typename Elem, typename RandomGenerator>
Elem choose(const std::vector<Elem> &vec, RandomGenerator& gen) {
  auto dis = std::uniform_int_distribution<>(0, vec.size() - 1);
  return vec[dis(gen)];
}

bool onTarget(const std::vector<int> &position, const std::vector<int> &targetX,
		 const std::vector<int> &targetY, const std::vector<int> &targetZ) {
  assert(targetX.size() == targetY.size() && targetY.size() == targetZ.size());
  for (size_t i = 0; i < targetX.size(); ++i) {
    if (position[0] == targetX[i] && position[1] == targetY[i] && position[2] == targetZ[i]) {
      return true;
    }
  }
  return false;
}

bool onLine(const std::vector<int> &position,
	    const std::vector<int> &linesY, const std::vector<int> &linesZ) {
  assert(linesY.size() == linesZ.size());
  for (size_t i = 0; i < linesY.size(); ++i) {
    if (position[1] == linesY[i] && position[2] == linesY[i]) {
      return true;
    }
  }
  return false;
}

std::vector<int> samplesWithoutReplacement(int max, int size) {
  std::random_device rd;
  std::mt19937 gen(rd());
  
  auto numbers = std::vector<int>(max);
  for (auto i = 0; i < max; ++i) {
    numbers[i] = i;
  }
  std::shuffle(numbers.begin(), numbers.end(), gen);
  numbers.resize(size);
  return numbers;
}

std::tuple<long, long> simulate(int avgSlidingLen, int numTargetLines) {
  const int sideLength = 100;

  const auto directions3D = std::vector<std::vector<int>>({ {0,0,1}, {0,1,0}, {1,0,0}, {0,0,-1}, {0, -1, 0}, {-1, 0, 0} }); 
  const auto directionsDetach = std::vector<std::vector<int>>({ {0,0,1}, {0,1,0}, {0,0,-1}, {0, -1, 0} }); 
  const auto directions1D = std::vector<std::vector<int>>({ {1,0,0}, {-1,0,0} }); 
 
  std::random_device rd;
  std::mt19937 gen(rd());

  // Compute parameters of distribution.
  const double a = avgSlidingLen + avgSlidingLen * avgSlidingLen;
  const double b = 1 - (a - avgSlidingLen)/a;
  // std::cout << a << "," << b << std::endl;
  std::negative_binomial_distribution<int> slidingDistribution(1, b);
  
  std::vector<int> numbers = samplesWithoutReplacement(10, 10);

  const auto linesY = samplesWithoutReplacement(sideLength, sideLength);
  const auto linesZ = samplesWithoutReplacement(sideLength, sideLength);

  const auto targetX = samplesWithoutReplacement(sideLength, numTargetLines);
  const auto targetY = samplesWithoutReplacement(sideLength, numTargetLines);
  const auto targetZ = samplesWithoutReplacement(sideLength, numTargetLines);

  auto uniform = std::uniform_int_distribution<>(0, sideLength - 1);
  auto position = std::vector<int>({uniform(gen), uniform(gen), uniform(gen)});

  int attachCounter = 0;
  long iterations3D = 0;  
  long iterations1D = 0;  
  int remainingSlidingIts = 0;
  bool isSliding = false;

  std::vector<int> curDirection = {0,0,0};
  while(!onTarget(position, targetX, targetY, targetZ)) {
    const bool isOnLine = onLine(position, linesY, linesZ);
    if (isOnLine && !isSliding) {
      remainingSlidingIts = getSlidingLength(avgSlidingLen, gen);
      isSliding = true;
      ++attachCounter;
    }
    if (remainingSlidingIts > 0) {
      assert(isOnLine);
      ++iterations1D;
      --remainingSlidingIts;
      curDirection = choose(directions1D, gen);
    } else {
      ++iterations3D;
      remainingSlidingIts = 0;
      if (isSliding) {
	// Just detached.
	isSliding = false;
	curDirection = choose(directionsDetach, gen);
      } else {
	curDirection = choose(directions3D, gen);
      }
    }

    // Adjust position.
    for (size_t i = 0; i < 3; ++i) {
      position[i] = (position[i] + curDirection[i]) % sideLength;
    }
  }
  return std::make_tuple(iterations1D, iterations3D);

}

int main(void) {
  //const std::vector<int> slidingLenGrid = {0, 5, 10, 30, 50, 100, 200, 300};
  const std::vector<int> slidingLenGrid = {5, 10, 30, 50, 100, 200, 300};
  //const std::vector<int> numLinesGrid = {1, 10, 30, 100};
  const std::vector<int> numLinesGrid = {10, 30, 100};
  const int numIt = 1000;
  
  auto results = std::vector<std::vector<long>>(slidingLenGrid.size() * numLinesGrid.size() * numIt);

  // Offset for each parameter config in results!
  size_t idxOffset = 0;
  for (auto avgSlidingLen : slidingLenGrid) {
    for (auto numLines : numLinesGrid) {
      std::cout << "Benchmarking avgSlidingLen = " << avgSlidingLen << ", numLines " << numLines << std::endl;
      // We parallelize here for better load-balancing.
      #pragma omp parallel for
      for (size_t i = 0; i < numIt; ++i) {
	long iterations1D, iterations3D;
	std::tie(iterations1D, iterations3D) = simulate(avgSlidingLen, numLines);
	const std::vector<long> curResult = {avgSlidingLen, numLines, iterations1D, iterations3D};
	results[idxOffset + i] = curResult;
      }
      idxOffset += numIt;
    }
  }

  // Write csv-output.
  std::cout << "Writing results.csv now!" << std::endl;
  // We start with the header:
  std::ofstream out("results.csv");
  const auto header = "avgSlidingLen,numLines,iterations1D,iterations3D";
  out << header << '\n';

  // Now all results!
  for (auto &result : results) {
    out << result[0] << ',' << result[1] << ',' << result[2] << ',' << result[3] << '\n';
  }
}
