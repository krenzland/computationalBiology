#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <random>
#include <tuple>
#include <vector>

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
    if (position[1] == linesY[i] && position[2] == linesZ[i]) {
      return true;
    }
  }
  return false;
}

int modulus(int a, int b) {
  return (b + (a%b)) % b;
}


template<typename RandomGenerator>
std::vector<int> samplesWithoutReplacement(int max, int size, RandomGenerator& gen) {
  auto numbers = std::vector<int>(max);
  for (auto i = 0; i < max; ++i) {
    numbers[i] = i;
  }
  std::shuffle(numbers.begin(), numbers.end(), gen);
  numbers.resize(size);
  return numbers;
}


template<typename RandomGenerator>
const std::pair<std::vector<int>, std::vector<int>> generateTargetLines(const std::vector<int> &linesY, const std::vector<int> &linesZ,
								       int numLines, RandomGenerator& gen) {
  assert(linesY.size() == linesZ.size());
  auto indices = samplesWithoutReplacement(linesY.size(), numLines, gen);
  auto targetsY = std::vector<int>(numLines);
  auto targetsZ = std::vector<int>(numLines);
  for (size_t i = 0; i < indices.size(); ++i) {
    targetsY[i] = linesY[indices[i]];
    targetsZ[i] = linesZ[indices[i]];
  }

  return std::make_pair(std::move(targetsY), std::move(targetsZ));
}


std::tuple<long, long> simulate(int avgSlidingLen, int numTargetLines) {
  const int sideLength = 100;

  // Set of all possible direcctions for movement.
  // DirectionsDetach is used to avoid a direct re-attachment.
  const auto directions3D = std::vector<std::vector<int>>({ {0,0,1}, {0,1,0}, {1,0,0}, {0,0,-1}, {0, -1, 0}, {-1, 0, 0} }); 
  const auto directionsDetach = std::vector<std::vector<int>>({ {0,0,1}, {0,1,0}, {0,0,-1}, {0, -1, 0} }); 
  const auto directions1D = std::vector<std::vector<int>>({ {1,0,0}, {-1,0,0} }); 
 
  std::random_device rd;
  std::mt19937 gen(rd());

  // Create 100 lines parallel to x-axis.
  const auto linesY = samplesWithoutReplacement(sideLength, sideLength, gen);
  const auto linesZ = samplesWithoutReplacement(sideLength, sideLength, gen);

  // Along those lines create numTargetLines targets.
  std::vector<int> targetsY, targetsZ;
  std::tie(targetsY, targetsZ) = generateTargetLines(linesY, linesZ, numTargetLines, gen);
  const auto targetsX = samplesWithoutReplacement(sideLength, numTargetLines, gen);

  for (int i = 0; i < numTargetLines; ++i) {
    bool isThere = false;
    for (int j = 0; j < sideLength; ++j) {
      isThere = isThere || ((targetsY[i] == linesY[j]) && (targetsZ[i] == linesZ[j]));
    } 
    assert(isThere);
  }

  // Initial position is random position in grid.
  auto uniform = std::uniform_int_distribution<>(0, sideLength - 1);
  auto position = std::vector<int>({uniform(gen), uniform(gen), uniform(gen)});

  // Setup counters and status variables.
  int attachCounter = 0;
  long iterations3D = 0;  
  long iterations1D = 0;  
  int remainingSlidingIts = 0;
  bool isSliding = false;
  std::vector<int> curDirection = {0,0,0};

  while(!onTarget(position, targetsX, targetsY, targetsZ)) {
    const bool isOnLine = onLine(position, linesY, linesZ);
    if (isOnLine && !isSliding) {
      // Initialise sliding here.
      remainingSlidingIts = getSlidingLength(avgSlidingLen, gen);
      isSliding = true;
      ++attachCounter;
    }
    if (remainingSlidingIts > 0) {
      // Currently sliding, 1D motion
      assert(isOnLine);
      ++iterations1D;
      --remainingSlidingIts;
      curDirection = choose(directions1D, gen);
    } else {
      // Not sliding, 3D motion
      ++iterations3D;
      if (isSliding) {
	// Just detached.
	isSliding = false;
	// Make sure to actually detach!
	curDirection = choose(directionsDetach, gen);
      } else {
	curDirection = choose(directions3D, gen);
      }
    }

    // Adjust position.
    for (size_t i = 0; i < 3; ++i) {
      position[i] =  modulus(position[i] + curDirection[i], sideLength);
      assert(position[i] >= 0 && position[i] < sideLength);
    }
  }

  // Make sure that we are on a line in the end.
  assert(onLine(position, linesY, linesZ));

  return std::make_tuple(iterations1D, iterations3D);

}

int main(void) {
  const std::vector<int> slidingLenGrid = {0, 5, 10, 30, 50, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200, 1300};
  const std::vector<int> numLinesGrid = {1, 10, 30, 50, 70, 100};
  const int numIt = 2 * 1024;
  
  auto results = std::vector<std::vector<long>>(slidingLenGrid.size() * numLinesGrid.size() * numIt);

  size_t idxOffset = 0; // Offset for each parameter config in results!
  for (auto avgSlidingLen : slidingLenGrid) {
    for (auto numLines : numLinesGrid) {
      std::cout << "Running avgSlidingLen = " << avgSlidingLen << ", numLines " << numLines << std::endl;

      // We parallelize here for better load-balancing.
      // Dynamic schedule with small chunksize is faster.
#pragma omp parallel for schedule(dynamic, 1)
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
