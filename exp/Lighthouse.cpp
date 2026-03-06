#include "Lighthouse.h"
#include "duf.hpp"
#include <fstream>
#include <sstream>
#include <deque>
#include <sstream>

#include <iostream>

std::string formatNumber(double number) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << number;
    return oss.str();
}

void print2dNummVector(vector<vector<numm> >& printMe)
{
	for (indd indx = 0; indx < printMe.size(); ++indx ) {
		vector<numm> tupleTempPrint = printMe[indx];
		for (indd indy = 0; indy < tupleTempPrint.size(); ++indy ) {
			cout << tupleTempPrint[indy] << " ";
		}
		cout << "\n";
	}
	cout << "Above is printMe\n";
}

// Function to print the contents of a vector
void printVector(const vector<numm>& vec) {
    cout << "[";
	for (size_t i = 0; i < vec.size(); ++i) {
        cout << vec[i];
        if (i != vec.size() - 1) {
            cout << ", ";
        }
    }
    cout << "]";
}

// Function to print the contents of an unordered_set of vectors
void printUnorderedSet(const unordered_set<vector<numm>, boost::hash<vector<numm>>>& uset) {
	int i = 0;
    for (const auto& vec : uset) {
        printVector(vec);
        cout << endl;
		i++;
    }
	cout << "Size of pattern set " << i << "\n";
}

// Function to print the contents of a vector<pair<numm, vector<numm>>>
void printVectorOfPairs(const vector<pair<numm, vector<numm>>>& vecOfPairs) {
    for (const auto& pair : vecOfPairs) {
        // Print the first element of the pair
        cout << pair.first << " : ";
        // Print the second element of the pair (which is a vector)
        printVector(pair.second);
        cout << endl;
    }
}

Lighthouse::Lighthouse(string filename, vector<indd> dataColumns, indd outcomeColumn, indd maxRules) // parse the data
{
	// Set rules count for bit array
	rulesCount = maxRules;

	// build dictionaries for attributes and fill them afterwards
	// here the data is transformed to multisets such that for each combination of eplanatory attributes the tuples per outcome as well as total count is stored
	vector<unordered_set<numm>> attributeValues(dataColumns.size(), unordered_set<numm>());
	keyToAttribute.resize(dataColumns.size());
	attributeToKey.resize(dataColumns.size());
	attributeNames.resize(dataColumns.size() + 1);

	//Read in the raw data...
	vector<vector<string>> raw(0);
	string line;
	ifstream data(filename);
	while (getline(data, line)) {
		stringstream lineStream(line);
		string cell;
		vector<string> lineVec;
		while (getline(lineStream, cell, ',')) { lineVec.push_back(cell); }
		raw.push_back(lineVec);
	}
	// Get attribute names of file
	for (indd j = 0; j < dataColumns.size(); ++j) {
		attributeNames[j] = raw[0][dataColumns[j]];
	}

	attributeNames[dataColumns.size()] = raw[0][outcomeColumn];
	for (indd i = 1; i < raw.size(); ++i) {
		string outcome = raw[i][outcomeColumn];
		if (outcomeToKey.count(outcome) == 0) {
			outcomeToKey.emplace(outcome, outcomeToKey.size());
			keyToOutcome.emplace(keyToOutcome.size(), outcome);
		}
		for (indd j = 0; j < dataColumns.size(); ++j) {
			numm attribute(stof(raw[i][dataColumns[j]]));
			if (attributeValues[j].count(attribute) == 0) {
				attributeValues[j].insert(attribute);
			}
		}
	}

	for (indd j = 0; j < dataColumns.size(); ++j) {
		vector<numm> values(attributeValues[j].begin(), attributeValues[j].end());
		std::sort(values.begin(), values.end());
		for (indd i = 0; i < values.size(); ++i) {
			keyToAttribute[j].emplace(i, values[i]);
			attributeToKey[j].emplace(values[i], i);
		}
	}

	// Create bit array for each tuple that is initially empty
	boost::dynamic_bitset<> tupleBitArrayTemp(0);

	for (indd i = 1; i < raw.size(); ++i) {
		vector<indd> tuple(dataColumns.size(), 0);
		vector<numm> tupleTemp(dataColumns.size(), 0);

		for (indd j = 0; j < dataColumns.size(); ++j) {
			tuple[j] = attributeToKey[j].find(stof(raw[i][dataColumns[j]]))->second;
			tupleTemp[j] = attributeToKey[j].find(stof(raw[i][dataColumns[j]]))->second;
		}
		auto ref = tuples.find(tuple);
		if (ref == tuples.end()) { ref = tuples.emplace(tuple, vector<indd>(keyToOutcome.size() + 2, 0)).first; }
		ref->second[keyToOutcome.size()] += 1;
		ref->second[outcomeToKey.find(raw[i][outcomeColumn])->second] += 1;

		// Push into tuple array
		tuples_new.push_back(tupleTemp);


		tupleT tempTupleT;
		tempTupleT.bitArray = tupleBitArrayTemp;
		tempTupleT.measure = stod(raw[i][outcomeColumn]);
		tempTupleT.estimate = 1;
		tuplesVectorNew.push_back(tempTupleT);
	}

	//print2dNummVector(tuples_new);

	N = tuples.size(); // number of tuples total
	D = dataColumns.size(); // number of dimensions
	O = outcomeToKey.size(); // number of outcome values

        // resultcounts = vector for how many tuples match a specific outcome value
	vector<indd> resultcounts(O, 0);


	totalN = 0;
	for (auto iter = tuples.begin(); iter != tuples.end(); ++iter) {
//                numm outcomeFound;
		// Check each outcome value
		for (indd o = 0; o < O; ++o) {
			resultcounts[o] += iter->second[o];

//                        if (iter->second[o] > 0) {
//				outcomeFound = o;
//                        }
		}
		totalN += iter->second[O];
//                cout << keyToOutcome[outcomeFound] << "\n"; // This is the actual outcome value
	}
        
        //DEBUGGING PURPOSES
//	for (indd o = 0; o < O; ++o) {
                //key to outcome value
//        	cout << "Outcome is " << keyToOutcome[o] << "\n";

               // how many tuples match that outcome value...
//	       	cout << "Result Counts is " << resultcounts[o] << "\n";
//       }

	// construct RCT for with default case as no rule exists yet
	vector<numm> rctBase(3 * O + 1);
	for (indd o = 0; o < O; ++o) {
		rctBase[o] = 1.0 / (numm)O;
		rctBase[O + o] = 0;
		rctBase[2 * O + o] = resultcounts[o];
	}
	rctBase[3 * O] = totalN;
	RCT.emplace(0, rctBase);


	lambdas = vector<numm>(0); // R_j -> Lambda_j
	lambdasNew = vector<numm>(0); // R_j -> Lambda_j


	vector<numm> pattern(2 * D + 3, 0);
	for (indd d = 0; d < D; ++d) {
		pattern[D + d] = attributeToKey[d].size() - 1;
	}
	pattern[2 * D + 1] = totalN;
	// 2*D = o or outcome, 2*D + 1 = totalN (support), 2*D+2 = resultcount

	// indd minRCount = min(resultcounts[0], resultcounts[1]); // add this so only the 0 rule shows up by default...
//	for (indd o = 0; o < O; ++o) { // Add default rules for each outcome, i.e. one rule matching all attrbiutes per outcome value
 	//	if (resultcounts[o] == minRCount) {
		//if (o > 0) {
//			vector<indd> p2 = pattern;
//			p2[2 * D] = o;
//			p2[2 * D + 2] = resultcounts[o];
//			addRuleNew(p2);
	//	}
//	}

	// Set initial rule estimates
	ruleEstimates = vector<numm>(rulesCount, 1.0);

	// Add default rule
	vector<numm> p2 = pattern;
	p2[2 * D] = 0;
	p2[2 * D + 2] = totalN; // resultcounts[0]; //
	addRuleNewDebug(p2);

    // cout << "Now doing iterative scaling" << "\n";
	//iterativeScaling(); //iterativeScalingNew();
	iterativeScalingNewRCT();
	//iterativeScaling();

	//cout << KLDivergence() << "   WOW \n";
}


bool Lighthouse::match(vector<indd>& tuple, vector<indd>& pattern) // test whether a tupple matches a pattern
{
	for (indd i = 0; i < D; ++i) {
		if (pattern[i] > tuple[i] || pattern[D+i] < tuple[i]) { return false; }
	}
	return true;
}

bool Lighthouse::match(vector<numm>& tuple, vector<indd>& pattern) // test whether a tupple matches a pattern
{
	for (indd i = 0; i < D; ++i) {
		if (pattern[i] > tuple[i] || pattern[D+i] < tuple[i]) { return false; }
	}
	return true;
}

bool Lighthouse::matchNew(vector<numm>& tuple, vector<numm>& pattern) // test whether a tupple matches a pattern
{
	for (indd i = 0; i < D; ++i) {
		if (pattern[i] > tuple[i] || pattern[D+i] < tuple[i]) { return false; }
	}
	return true;
}

bool Lighthouse::patternsEqual(vector<indd>& pattern1, vector<indd>& pattern2) // test whether two patterns are equal
{
	for (indd i = 0; i < D; ++i) {
		if (pattern1[i] != pattern2[i] || pattern1[D+i] != pattern2[D+i]) { return false; }
	}
	if (pattern1[D] != pattern2[D]) { return false; }
	return true;
}

bool Lighthouse::patternsEqualNew(vector<numm>& pattern1, vector<numm>& pattern2) // test whether two patterns are equal
{
	for (indd i = 0; i < D; ++i) {
		if (pattern1[i] != pattern2[i] || pattern1[D+i] != pattern2[D+i]) { return false; }
	}
	if (pattern1[D] != pattern2[D]) { return false; }
	return true;
}

void Lighthouse::addRuleNewDebug(vector<numm>& rule) // insert a rule into the Explanation table
{
	// cout << "\nStart new add rule method\n";

	// Calculate number of tuples that match rule r
	numm tuplesMatchR = 0;

	// Calculate average value of measure attribute of tuples that match r
	numm expectedMeasureAverage = 0;

	// For all tuples t in dataset D
	for (indd tupleIndex = 0; tupleIndex < tuples_new.size(); ++tupleIndex){
		vector<numm> alocTup = tuples_new[tupleIndex];
		
		// If tuple matches rule
		if (matchNew(alocTup, rule)) {
			// Change bit entry in t.BA[i] to 1. tupleIndex is current tuple
			tuplesVectorNew[tupleIndex].bitArray.push_back(1);

			expectedMeasureAverage += tuplesVectorNew[tupleIndex].measure;
			tuplesMatchR += 1;
		} else {
			tuplesVectorNew[tupleIndex].bitArray.push_back(0);
		}
	}

	// Need to recompute the RCT
	RCT_New.clear();

	// Group by t.BA and aggregate over COUNT(*), SUM(t[m]), and SUM(t[m^]) to compute RCT
	auto groups = group_by(tuplesVectorNew, &tupleT::bitArray);

	for (auto iter = groups.begin(); iter != groups.end(); ++iter) {
		boost::dynamic_bitset<> bitArrayTemp = get<0>(iter->first);
		vector<tupleT> tuplesVectorTempGroup = iter->second;
		numm sumMeasure = 0;
		numm sumEstimate = 0;
		for_each(tuplesVectorTempGroup .begin(), tuplesVectorTempGroup.end(), [&](tupleT i) {
			sumMeasure += i.measure;
			sumEstimate += i.estimate;
		});

		//cout << bitArrayTemp <<" is the bit array\n";
		//cout << tuplesVectorTempGroup.size() <<" is the SIZE\n";
		//cout << sumMeasure <<" is the sum measure\n";
		//cout << sumEstimate <<" is the sum estimate\n";

//		auto oldRef = RCT_New.find(bitArrayTemp);
//		if (oldRef != RCT_New.end()) {
//			cout << "HEY I FOUND A OLD REF\n";
//			oldRef->second[0] = tuplesVectorTempGroup.size();
//			oldRef->second[1] = sumMeasure;
//			oldRef->second[2] = sumEstimate;
//		}
//		else {

		//cout << "HEY I ADDED A NEW REF\n";

		vector<numm> rctBaseTemp(3);
		rctBaseTemp[0] = tuplesVectorTempGroup.size();
		rctBaseTemp[1] = sumMeasure;
		rctBaseTemp[2] = sumEstimate;
		RCT_New.emplace(bitArrayTemp, rctBaseTemp);
//		}
	}

	expectedMeasureAverage = expectedMeasureAverage/tuplesMatchR;
	ruleMeasures.push_back(expectedMeasureAverage);
        //cout << "Tuples match is " << tuplesMatchR << "\n";
        //cout << "Average Expected Measure is " << expectedMeasureAverage << "\n";

	// Create bit array for each rule that has one in ith entry
//	boost::dynamic_bitset<> ruleBitArrayTemp(rulesCount);
//	ruleBitArrayTemp[patterns.size()] = 1;
//	ruleBitArray.push_back(ruleBitArrayTemp);
 //   	for (boost::dynamic_bitset<>::size_type i = 0; i < rulesCount; ++i)
 //       	cout << ruleBitArrayTemp[i];
//	cout << "Above is ruleBitArray\n";

	patterns.push_back(rule);
	lambdas.push_back(0);
	lambdasNew.push_back(1);

	// cout << "Done adding a new rule\n";
}


template <class any>
long double vectorProduct(vector<any> vec) {
  long double total = 1;

  for(any num : vec) {
    total *= num;
  }

  return total;
}

void Lighthouse::iterativeScalingNewRCT() // perform iterative scaling with RCT as described in the paper
{
	// cout << "\nStart new iterative scaling RCT method\n";

	int rules_size = patterns.size();

	// DIFF array (this is line 8 of algorithm 3)
	numm diffArray[rules_size]; // Entries are zero-initialized

	// This is to keep track of rule measure estimates (i.e., average value of tuple estimates matching rule)
	numm ruleMeasureEstimates[rules_size]; // Entries are zero-initialized

	numm epsilon = 0.01;

	bool divergent = true;
	while (divergent) {

        // For rules already generated...
		for (indd p = 0; p < rules_size; ++p) {

			//cout << "DEBUG - PATTERN: ";
			for (indd i = 0; i < D; ++i) {
				// cout << patterns[p][i] << " ";
			}
			//cout << "with measure value " << ruleMeasures[p] << "\n";
     
			numm sumSumEstimates = 0;
			numm sumCount = 0;

    			for (auto i = RCT_New.begin(); i != RCT_New.end(); i++) {
				if (i->first[p] == 1) {
					sumCount += i->second[0];
					sumSumEstimates += i->second[2];
				}
			}

			ruleMeasureEstimates[p] = sumSumEstimates/sumCount;
			diffArray[p] = abs(ruleMeasures[p] - ruleMeasureEstimates[p]) / abs(ruleMeasures[p]) ;

			//cout << "M is: " << ruleMeasures[p] << "\n";
			//cout << "Estimate M is: " << sumSumEstimates/sumCount << "\n";
			//cout << "DIFF IS : " << diffArray[p] << "\n";

		}
		numm nextDiff = *std::max_element(diffArray, diffArray + rules_size);
		// cout << "NEXT DIFF: " << nextDiff <<"\n";

		if (nextDiff > epsilon) {
			indd indexNextDiff = find(diffArray, diffArray + rules_size, nextDiff) - diffArray;

			numm lambdaOld = lambdasNew[indexNextDiff];
			lambdasNew[indexNextDiff] = lambdasNew[indexNextDiff] * (ruleMeasures[indexNextDiff] / ruleMeasureEstimates[indexNextDiff]);

			//cout << "here is lambdaOld " << lambdaOld << "\n";
			//cout << "here is lambdaNew " << lambdasNew[indexNextDiff] << "\n";

			// For each RCT that have next-ith bit of the bit array set to 1, recompute sum of measure estimates
    			for (auto i = RCT_New.begin(); i != RCT_New.end(); i++) {
				if (i->first[indexNextDiff] == 1) {
					i->second[2] = i->second[2] * (lambdasNew[indexNextDiff] / lambdaOld );
				}
			}

		} else {
			numm vp = vectorProduct(lambdasNew);
			//cout << "HERE IS VP " << vp << "\n";
			//cout << "HERE IS lambdas " << "\n";
			//printVector(lambdasNew);
			//cout << "\n";

			// For all tuples t in dataset D, update tuple measure estimates
			for (indd tupleIndex = 0; tupleIndex < tuples_new.size(); ++tupleIndex){

				numm t_m = 1;
				// Iterate over the bit array in reverse
				for (indd i = 0; i < rules_size; ++i) {
					// Get the current bit from the bit array
					if (tuplesVectorNew[tupleIndex].bitArray[i] == 1) {
						t_m *= lambdasNew[i];
					}
				}
				tuplesVectorNew[tupleIndex].estimate = t_m;
				// cout << tuplesVectorNew[tupleIndex].bitArray << " ";
				//ruleEstimates[tupleIndex] = vp;
			}
			cout << "\n";
			divergent = false;
			break;
		}
	}
}

void Lighthouse::iterativeScaling() // perform iterative scaling as described in the paper
{
	numm limit = 0.1 / (numm)totalN;
	bool divergent = true;
	while (divergent) {
		divergent = false;

                // For rules already generated...
		for (indd p = 0; p < patterns.size(); ++p) {
			indd outcome = patterns[p][2*D];
			numm percentageApplicableTuples = (numm)patterns[p][2*D + 2] / (numm)totalN;
			numm expectedSum = 0;
			numm deriv = 0;
			numm delta = 0;
                        //cout << "RCT BEGIN ";

			for (auto iter = RCT.begin(); iter != RCT.end(); ++iter) {
                        //        cout << "RCT FIRST " << iter->first;

				if (iter->first & ((indd)1 << p)) {
					numm term = (iter->second[3 * O] / (numm)totalN) * iter->second[outcome] * exp(delta * iter->second[O + outcome]);
					expectedSum += term;
					deriv += iter->second[O + outcome] * term;
				}
			}
                        //cout << "RCT STOP ";

			while (abs(expectedSum - percentageApplicableTuples) > limit && abs(delta) < 50) {
				divergent = true;
				if (abs((expectedSum - percentageApplicableTuples) / deriv) > 100) {
					deriv*=100;
				}
				delta -= (expectedSum - percentageApplicableTuples) / deriv;
				expectedSum = 0;
				deriv = 0;
				for (auto iter = RCT.begin(); iter != RCT.end(); ++iter) {
					if (iter->first & ((indd)1 << p)) {
						numm term = (iter->second[3 * O] / (numm)totalN) * iter->second[outcome] * exp(delta * iter->second[O + outcome]);
						expectedSum += term;
						deriv += iter->second[O + outcome] * term;
					}
				}
			}
			lambdas[p] += delta;
			if (delta != 0) {
				for (auto iter = RCT.begin(); iter != RCT.end(); ++iter) {
					if (iter->first & ((indd)1 << p)) {
						numm denominator = 0;
						vector<numm> numerator(O, 0);
						for (indd o = 0; o < O; ++o) {
							numm lambdaSum = 0;
							for (indd p2 = 0; p2 < patterns.size(); ++p2) {
								if ((patterns[p2][2*D] == o) && (iter->first & ((indd)1 << p2))) {
									lambdaSum += lambdas[p2];
								}
							}
							numerator[o] = exp(lambdaSum);
							denominator += exp(lambdaSum);
						}
						for (indd o = 0; o < O; ++o) {
							iter->second[o] = numerator[o] / denominator;
						}
					}
				}
			}
		}
	}
}

/**
 * @brief This function normalizes a container so that it sums to 1.0.
 *
 * @param begin The beginning of the range to normalize.
 * @param end The end of the range to normalize.
 * @param out The beginning of the output range (can be the same as begin).
 */
template <typename InputIterator, typename OutputIterator>
void normalizeProbability(InputIterator begin, InputIterator end, OutputIterator out) {
    double norm = static_cast<double>(std::accumulate(begin, end, 0.0));
    std::transform(begin, end, out, [norm](decltype(*begin) t){ return t/norm; });
}

numm Lighthouse::KLDivergenceNew() // measure KL divergence of current Explanation Table to empiricial distribution
{
	vector<numm> normalizedMeasure(tuplesVectorNew.size(), 0);
	vector<numm> normalizedMeasureEstimate(tuplesVectorNew.size(), 0);

	// For all tuples t in dataset D, get tuple measure and estimates
	for (indd tupleIndex = 0; tupleIndex < tuplesVectorNew.size(); ++tupleIndex){
		normalizedMeasure[tupleIndex] = tuplesVectorNew[tupleIndex].measure;
		normalizedMeasureEstimate[tupleIndex] = tuplesVectorNew[tupleIndex].estimate;
	}
	//vector<numm> normalizedMeasure(ruleMeasures);  
	//vector<numm> normalizedMeasureEstimate(ruleEstimates);  

	normalizeProbability(normalizedMeasure.begin(), normalizedMeasure.end(), normalizedMeasure.begin());
	normalizeProbability(normalizedMeasureEstimate.begin(), normalizedMeasureEstimate.end(), normalizedMeasureEstimate.begin());

        //cout << "TESTING " << std::accumulate(ruleMeasures.begin(), ruleMeasures.end(),
        //                        decltype(ruleMeasures)::value_type(0)) << "\n";

	numm sum = 0;
	numm indexTemp = 0;
	for (auto iter = tuples.begin(); iter != tuples.end(); ++iter) {
		sum += normalizedMeasure[indexTemp] * log(normalizedMeasure[indexTemp]/normalizedMeasureEstimate[indexTemp]);
		indexTemp += 1;
	}
	return sum;
}


numm Lighthouse::KLDivergence() // measure KL divergence of current Explanation Table to empiricial distribution
{
	numm sum = 0;
//	numm eps = 1e-15;
	for (auto iter = tuples.begin(); iter != tuples.end(); ++iter) {
		numm total = (numm)iter->second[O];
		for (indd o = 0; o < O; ++o) {
			numm pi = (numm)iter->second[o] / (numm)iter->second[O];
			numm qi = RCT.find(iter->second[O + 1])->second[o];
			//pi = pi + eps;
			//qi = qi + eps;
			if ((pi != 0) && (qi != 0)) { sum += iter->second[O] * (pi*log(pi / qi)); }

			// alternative method of computation to compare with IDS and decision trees: 
//			numm qi = RCT.find(iter->second[O + 1])->second[o] + eps;
//			numm pi = 1 + eps;
//			numm cnt = (numm)iter->second[o];
//			sum += (cnt * (pi * log(pi / qi))); 

//			pi = eps;
//			cnt = total - cnt;
//			sum += (cnt * (pi * log(pi / qi)));
		}
	}
	return sum;
}

numm Lighthouse::evalStraight(vector<indd>& pattern, vector<indd>& bestPattern, numm& bestGain) // selects best outcome for pattern and stores it in bestPattern
{
	numm thisGain = 0;
	indd sup = 0;
	vector<indd> hits(O, 0);
	vector<numm> model(O, 0);
	for (auto iter = tuples.begin(); iter != tuples.end(); ++iter) {
		vector<indd> tuple = iter->first;
		if (match(tuple, pattern)) {
			sup += iter->second[O];
			for (indd o = 0; o < O; ++o) {
				hits[o] += iter->second[o];
				model[o] += iter->second[O] * RCT.find(iter->second[O + 1])->second[o];
			}
		}

	}
	for (indd o = 0; o < O; ++o) {
		numm supportF = sup;
		numm trueRate = (numm)hits[o] / supportF;
		numm expRate = model[o] / supportF;
		if ((trueRate != 0) && (expRate != 0)) {
			numm gain = supportF * trueRate *log(trueRate / expRate);
			for (indd o2 = 0; o2 < O; ++o2) {
				if (o2 != o) {
					numm tr = (numm)hits[o2] / supportF;
					numm oldER = model[o2] / supportF;
					numm newER = (oldER / (1.0 - expRate)) * (1.0 - trueRate);
					if ((tr != 0) && (oldER != 0) && (newER != 0)) {
						gain += supportF * tr * (log(tr / oldER) - log(tr / newER));
					}
				}
			}
			thisGain = max(thisGain, gain);
			if (gain > bestGain) {
				bestGain = gain;
				bestPattern = pattern;
				bestPattern.push_back(o);
				bestPattern.push_back(sup);
				bestPattern.push_back(hits[o]);
			}
		}
	}
	return thisGain;
}


void Lighthouse::generateSample(vector<vector<indd>>& sample, indd sampleSize) { // draw a sample of desired size from the data weighted by total occurence
	// Because we transformed teh data to a multiset, naive sample from "tuples" neglects more common items and is not a fair sample from the original data.
	// So we generate number between 0 and N and accumulate the tuple count in order until the cumulative um reaches the randum count, this yields a fair randomly drawn tuple
	indd realSampleSize = min(totalN, sampleSize);
	random_device rd;
	mt19937_64 gen(rd());
	vector<indd> thresholds(totalN);
	for (indd i = 0; i < totalN; ++i) { thresholds[i] = i; }
	for (indd i = 0; i < realSampleSize; ++i) {
		uniform_int_distribution<indd> indexGen(i, totalN - 1);
		indd index = indexGen(gen);
		swap(thresholds[i], thresholds[index]);
	}
	thresholds.resize(realSampleSize);
	sort(thresholds.begin(), thresholds.end());
	indd s = 0;
	indd cumulative = 0;
	for (auto iter = tuples.begin(); iter != tuples.end() && s < realSampleSize; ++iter) {
		cumulative += iter->second[O];
		if (cumulative >= thresholds[s]) {
			sample[s] = iter->first;
			++s;
		}
	}
	sample.resize(s);
}

void Lighthouse::addSimpleAncestors(vector<indd>& protoPattern, unordered_map<vector<indd>, vector<numm>, ind_vector_hasher>& candidates, indd depth, vector<indd>& data)
{
	// generate ancestor patterns and att them to the candidate list (or update their counts if they already exist)
	if (depth < D) {
		if (protoPattern[depth] == protoPattern[D + depth]) {
			indd temp = protoPattern[depth];
			protoPattern[depth] = 0;
			protoPattern[D + depth] = attributeToKey[depth].size() - 1;
			addSimpleAncestors(protoPattern, candidates, depth + 1, data);
			protoPattern[depth] = temp;
			protoPattern[D + depth] = temp;
		}
		addSimpleAncestors(protoPattern, candidates, depth + 1, data);
	}
	else {
		auto ref = candidates.find(protoPattern);
		if (ref == candidates.end()) {
			ref = candidates.emplace(protoPattern, vector<numm>(2 * O + 1, 0)).first;
		}
		for (indd o = 0; o < O; ++o) {
			ref->second[2 * o] += data[o];
			ref->second[2 * o + 1] += (numm)data[O] * RCT.find(data[O + 1])->second[o];
		}
		ref->second[2 * O] += data[O];
	}
}

void Lighthouse::generateSampleNew(vector<vector<numm>>& sample, indd sampleSize) { // draw a sample of desired size from the data
	// cout << "Run generateSampleNew\n";

	indd realSampleSize = min(totalN, sampleSize);
	random_device rd;
	mt19937_64 gen(rd());
	uniform_int_distribution<indd> indexGen(0, totalN - 1);

	for (indd index = 0; index < realSampleSize; ++index){
		sample[index] = tuples_new[indexGen(gen)];
	}
	sample.resize(realSampleSize);
}

void Lighthouse::addSimpleAncestorsNew(vector<numm>& protoPattern, unordered_set<vector<numm>, boost::hash<std::vector<numm>>>& candidates, indd depth)
{
	// generate ancestor patterns and att them to the candidate list (or update their counts if they already exist)
	if (depth < D) {
		if (protoPattern[depth] == protoPattern[D + depth]) {
			indd temp = protoPattern[depth];
			protoPattern[depth] = 0;
			protoPattern[D + depth] = attributeToKey[depth].size() - 1;
			addSimpleAncestorsNew(protoPattern, candidates, depth + 1);
			protoPattern[depth] = temp;
			protoPattern[D + depth] = temp;
		}
		addSimpleAncestorsNew(protoPattern, candidates, depth + 1);
	}
	else {
		auto ref = candidates.find(protoPattern);
		if (ref == candidates.end()) {
			candidates.insert(protoPattern);
			//ref = candidates.emplace(protoPattern, vector<numm>(2, 0)).first;
		}
	}
}


Lighthouse::~Lighthouse()
{
}

bool Lighthouse::originalFlashlight(indd numberRules, indd sampleSize, numm minSupport, bool removeInactivatedPatterns, bool verbose)
{
	// internal counting gain and time for verbose mode
	auto globalStart = chrono::high_resolution_clock::now();
	numm baseKL = KLDivergenceNew();
	numm recentKL = baseKL;

	vector<numm> normalizedMeasure(tuplesVectorNew.size(), 0);
	vector<numm> normalizedMeasureEstimate(tuplesVectorNew.size(), 0);

	// cout << "START FLASHLIGHT PART \n";

	for (indd rules = 0; rules < numberRules; ++rules) {
		auto start = chrono::high_resolution_clock::now();

		// Do the normalizing stuff here...

		//normalizedMeasure.clear();
		//normalizedMeasureEstimate.clear();

		// For all tuples t in dataset D, get tuple measure and estimates
		for (indd tupleIndex = 0; tupleIndex < tuplesVectorNew.size(); ++tupleIndex){
			normalizedMeasure[tupleIndex] = tuplesVectorNew[tupleIndex].measure;
			normalizedMeasureEstimate[tupleIndex] = tuplesVectorNew[tupleIndex].estimate;
		}

		//cout << "CHECK MEASURE ESTIMATE" << "\n";
		//cout << tuplesVectorNew.size() << normalizedMeasureEstimate.size() << "\n";
		//printVector(normalizedMeasureEstimate);
		// cout << "\n";

		normalizeProbability(normalizedMeasure.begin(), normalizedMeasure.end(), normalizedMeasure.begin());
		normalizeProbability(normalizedMeasureEstimate.begin(), normalizedMeasureEstimate.end(), normalizedMeasureEstimate.begin());


		// draw sample for flashlight
		vector<vector<numm>> sample(sampleSize); // vector<vector<indd>> sample(sampleSize);
		generateSampleNew(sample, sampleSize); //generateSample(sample, sampleSize);

		// generate LCA table immediatly (no aggregates?) [cf. Table 7 in El Gebaly et al. 2014]
		//unordered_map<vector<numm>, vector<numm>, numm_vector_hasher> patternCandidates; // [AttributesLow, AttributesHigh] -> [Out1Count, Out1Exp, Out2Count, ..., OutxExp, TotalCount]

		unordered_set<vector<numm>, boost::hash<std::vector<numm>>> patternCandidates;

		for (indd tupleIndex = 0; tupleIndex < tuples_new.size(); ++tupleIndex){
			vector<numm> alocTup = tuples_new[tupleIndex];
			for (indd s = 0; s < sample.size(); ++s) {
				vector<numm> protoPattern(2 * D + 3, 0);

				for (indd k = 0; k < D; ++k) {
					if (alocTup[k] == sample[s][k]) {
						protoPattern[k] = sample[s][k];
						protoPattern[D + k] = sample[s][k];
					}
					else {
						protoPattern[k] = 0;
						protoPattern[D + k] = attributeToKey[k].size() - 1;
					}
				}
				addSimpleAncestorsNew(protoPattern, patternCandidates, 0);
			}
		}

		// cout << "patternCandidates contains:" << endl;
        // printUnorderedSet(patternCandidates);

		// evalaute gain of ancestor patterns, i.e., correct aggregates cf. [Table 8 in El Gebaly et al. 2014] and [Sec. 3.2 in Vollmer et al. 2019]
		vector<pair<numm, vector<numm>>> topTemplates;
		numm minGain = 0;
		for (auto iter = patternCandidates.begin(); iter != patternCandidates.end(); ++iter) {
			numm matchInSample = 0;
			vector<numm> allocatedTuple = (*iter); //->first;

			if (removeInactivatedPatterns) {
				bool inactivatedPattern = false;
				// std::stringstream stream;
				for (indd d = 0; d < D; ++d) {
					// stream << attributeNames[d] << "=" << allocatedTuple[d] << ", ";
					if ((allocatedTuple[d] == 0) && (allocatedTuple[D + d] == 0)) {
						inactivatedPattern = true;
						break;
					}
				}
				if (inactivatedPattern) {
					// cout << stream.str() << "\n";
					continue;
				}
			}

			numm maxGain = 0;

			// Need to calculate gain differently..

			numm gain = 0;
			numm indexTemp = 0;
			numm supportCheck = 0;

			numm gain_log_numerator = 0;
			numm gain_log_denominator = 0;
			// For all tuples t in dataset D
			for (indd tupleIndex = 0; tupleIndex < tuples_new.size(); ++tupleIndex){
				if (matchNew(tuples_new[tupleIndex], allocatedTuple)) {
					supportCheck += 1;
					gain_log_numerator += normalizedMeasure[tupleIndex];
					gain_log_denominator += normalizedMeasureEstimate[tupleIndex];
				}
			}

			for (indd tupleIndex = 0; tupleIndex < tuples_new.size(); ++tupleIndex){
				if (matchNew(tuples_new[tupleIndex], allocatedTuple)) {
					// 	cout << "\n" << "measure " << normalizedMeasure[tupleIndex];
					// 	cout << "measure estimate " << normalizedMeasureEstimate[tupleIndex] << "\n";
					// gain += normalizedMeasure[tupleIndex] * log(normalizedMeasure[tupleIndex]/normalizedMeasureEstimate[tupleIndex]);
					gain += normalizedMeasure[tupleIndex] * log(gain_log_numerator/gain_log_denominator);
				}
			}


			numm supportRate = supportCheck / (numm) totalN;
			allocatedTuple[2 * D + 1] = supportCheck;


			//cout << "\n" << "GAIN " << gain;
			//cout << "\n" << "SupportRate " << supportRate << "\n";


			if (supportRate >= minSupport) {
				maxGain = max(maxGain, gain);
			}

			// END new way of calculating GAIN here!!!

			if (maxGain > minGain) {
				topTemplates.emplace_back(maxGain, allocatedTuple);
				if (topTemplates.size() > 10) {
					sort(topTemplates.rbegin(), topTemplates.rend());
					topTemplates.resize(10);
					minGain = topTemplates[9].first;
				}
			}
		}

		//cout << "topTemplates contains:" << endl;
    	//printVectorOfPairs(topTemplates);

		if (topTemplates.size() == 0) {
			cout << "No pattern candidates generated, should end the process!" << "\n";
			return false;
		}

		// // insert best pattern into the explanation table
		// vector<numm> bestPattern = topTemplates[0].second;

		// // If the pattern is already added to the table, the process should end: 
		// for (indd p = 0; p < patterns.size(); ++p) {
		// 	  vector<numm> pat = patterns[p];
		// 	  if (patternsEqualNew(pat, bestPattern)) {
		// 		  cout << "New pattern already exists in the table, should end the process!" << "\n";
		// 		  bestPattern = topTemplates[2].second;
		// 		  return false;
		// 	  }
		// }

		// Best pattern to insert into the explanation table
    	vector<numm> bestPattern;

		// Flag to indicate if a new pattern has been found
		bool foundNewPattern = false;

		// Iterate over topTemplates to find a new pattern
		for (indd i = 0; i < topTemplates.size(); ++i) {
			vector<numm> candidatePattern = topTemplates[i].second;
			bool alreadyExists = false;

			// Check if the candidate pattern is already in patterns
			for (indd p = 0; p < patterns.size(); ++p) {
				if (patternsEqualNew(patterns[p], candidatePattern)) {
					alreadyExists = true;
					break;
				}
			}

			// If the pattern does not already exist, use it as the bestPattern
			if (!alreadyExists) {
				bestPattern = candidatePattern;
				foundNewPattern = true;
				break;
			}
		}

		if (!foundNewPattern) {
			cout << "No new pattern found, all of them already exist in the table." << endl;
			return false;
		}

		// vector<numm> bestPattern = topTemplates[0].second;

		// // If the pattern is already added to the table, the process should end: 
		// for (indd p = 0; p < patterns.size(); ++p) {
		// 	  vector<numm> pat = patterns[p];
		// 	  if (patternsEqualNew(pat, bestPattern)) {
		// 		  cout << "New pattern already exists in the table, should end the process!" << "\n";
		// 		  return false;
		// 	  }
		// }

		addRuleNewDebug(bestPattern);
		iterativeScalingNewRCT();

		auto end = chrono::high_resolution_clock::now();
		if (verbose) {
			numm currentKL = KLDivergenceNew();
			// cout << "FL Original " << sampleSize << "," << rules + 1 << "," << chrono::duration_cast<chrono::milliseconds>(end - start).count()<<"," << chrono::duration_cast<chrono::milliseconds>(end - globalStart).count() << ",";
			// cout << recentKL - currentKL << "," << baseKL - currentKL << "," << currentKL << "\n";
			recentKL = currentKL;
		}
		return true;
	}
	return false;
}

void Lighthouse::printTable(numm baseKL, string fileName) // print explanation table to console
{
	ofstream file;
	file.open(fileName, ofstream::trunc);

	numm KL = KLDivergence();
	numm gain = baseKL - KL;
	// cout << "\n===[Table Size: " << patterns.size() << " | Base KL-Div: " << baseKL << " | KL-Div: " << KL << " | Gain: " << gain << "]===\n";
	for (indd d = 0; d < D; ++d) {
		// cout<<attributeNames[d]<<",";
		file << attributeNames[d] << ";";
	}
	// cout<<attributeNames[D]<<"\n";
	// file << attributeNames[D] << ";support;confidence" << "\n" << flush;
	file << attributeNames[D] << ";support" << "\n" << flush;

	for (indd p = 0; p < patterns.size(); ++p) {
		for (indd d = 0; d < D; ++d) {
			if ((patterns[p][d] == 0) && (patterns[p][D + d] == keyToAttribute[d].size() - 1)) { // && (p >= O)) {
				// cout << "[*],";
				//if (p >= O) {   // if not among two first general distribution patterns
				file << "*;";
				//}
			} 
			else {
				numm featureValue = 0;
				numm firstValue = keyToAttribute[d].find(patterns[p][d])->second;
				numm secondValue = keyToAttribute[d].find(patterns[p][D+d])->second;
				if (firstValue == secondValue) {
					featureValue = firstValue;
				}
				else {     // handling the binned features case where more than one feature value may be possible in a pattern
					featureValue = ((secondValue - firstValue) / 2);
				}

				// cout << "[" << firstValue << "-" << secondValue << "],";
				if (p >= O) {
                                        if (firstValue == secondValue) {
					    file << featureValue << ";";
                                        }
                                        else {
                                            file << firstValue << "," << secondValue << ";"; // NEW ADDITION
                                        }
				}
                                else {
                                        if (firstValue == secondValue) {
					    file << featureValue << ";";
                                        }
                                        else {
                                            file << firstValue << "," << secondValue << ";"; // NEW ADDITION
                                        }
                                }
			}
		}

		//string outcome = std::to_string(ruleMeasures[p]);     //keyToOutcome.find(patterns[p][2 * D])->second;
		string outcome = formatNumber(ruleMeasures[p]);
		indd support = patterns[p][2 * D + 1];
		numm supportPercentage = (numm)support; // / (numm)totalN;
		numm confidence = (numm)patterns[p][2 * D + 2] / (numm)support;

		// cout << outcome << "  ; Sup: " << support << " Prec: " << confidence << "\n";
		//if (p >= O) {
		//file << outcome << ";" << supportPercentage << ";" << confidence << "\n" << flush;
		file << outcome << ";" << supportPercentage << "\n" << flush;
		//}
	}
}

string Lighthouse::getTable() //returns the entire explanation table as string
{
	string result = "";
	result += "\n===[Table Size: " + to_string(patterns.size()) + " | KL-Div: " + to_string(KLDivergence()) + "]===\n";
	for (indd d = 0; d < D; ++d) {
		result += attributeNames[d] + ",";
	}
	result += attributeNames[D] + "\n";
	for (indd p = 0; p < patterns.size(); ++p) {
		for (indd d = 0; d < D; ++d) {
			if ((patterns[p][d] == 0) && (patterns[p][D + d] == keyToAttribute[d].size() - 1) && (p >= O)) {
				result += + "[*],";
			}
			else {
				result += + "[" + to_string(keyToAttribute[d].find(patterns[p][d])->second) + "-" + to_string(keyToAttribute[d].find(patterns[p][D + d])->second) + "],";
			}
		}
		//result += keyToOutcome.find(patterns[p][2 * D])->second + "  ; Sup: " + to_string(patterns[p][2 * D + 1]) + " Prec: " + to_string((numm)patterns[p][2 * D + 2] / (numm)patterns[p][2 * D + 1]) + "\n";
		result += std::to_string(ruleMeasures[p]) + "  ; Sup: " + to_string(patterns[p][2 * D + 1]) + " Prec: " + to_string((numm)patterns[p][2 * D + 2] / (numm)patterns[p][2 * D + 1]) + "\n";
	}
	return result;
}

void Lighthouse::LENS(indd numberRules, bool verbose, indd samplesize, indd FLCandidates, indd mode, numm minSupport)
{
	const indd patternCutoff = 6; // define limit for dimensions of pattern expansion, i.e., if a simple pattern from Flashlight has 6 or more constants it is not considered for expansion
	// internal counting gain and time for verbose mode
	auto globalStart = chrono::high_resolution_clock::now();
	numm baseKL = KLDivergenceNew();
	numm recentKL = baseKL;

	vector<numm> normalizedMeasure(tuplesVectorNew.size(), 0);
	vector<numm> normalizedMeasureEstimate(tuplesVectorNew.size(), 0);

	for (indd rules = 0; rules < numberRules; ++rules) {
		auto start = chrono::high_resolution_clock::now();

		// Do the normalizing stuff here...

		//normalizedMeasure.clear();
		//normalizedMeasureEstimate.clear();

		// For all tuples t in dataset D, get tuple measure and estimates
		for (indd tupleIndex = 0; tupleIndex < tuplesVectorNew.size(); ++tupleIndex){
			normalizedMeasure[tupleIndex] = tuplesVectorNew[tupleIndex].measure;
			normalizedMeasureEstimate[tupleIndex] = tuplesVectorNew[tupleIndex].estimate;
		}
		normalizeProbability(normalizedMeasure.begin(), normalizedMeasure.end(), normalizedMeasure.begin());
		normalizeProbability(normalizedMeasureEstimate.begin(), normalizedMeasureEstimate.end(), normalizedMeasureEstimate.begin());

		// END NORMALIZING STUFF

		// draw sample for flashlight
		vector<vector<numm>> sample(samplesize);
		generateSampleNew(sample, samplesize);

		// generate LCA table immediatly with aggregates [cf. Table 7 in El Gebaly et al. 2014]
		unordered_set<vector<numm>, boost::hash<std::vector<numm>>> patternCandidates;

		for (indd tupleIndex = 0; tupleIndex < tuples_new.size(); ++tupleIndex){
			vector<numm> alocTup = tuples_new[tupleIndex];
			for (indd s = 0; s < sample.size(); ++s) {
				vector<numm> protoPattern(2 * D + 3, 0);

				for (indd k = 0; k < D; ++k) {
					if (alocTup[k] == sample[s][k]) {
						protoPattern[k] = sample[s][k];
						protoPattern[D + k] = sample[s][k];
					}
					else {
						protoPattern[k] = 0;
						protoPattern[D + k] = attributeToKey[k].size() - 1;
					}
				}
				addSimpleAncestorsNew(protoPattern, patternCandidates, 0);
			}
		}

		vector<pair<numm, vector<numm>>> topTemplates;
		numm minGain = 0;
		for (auto iter = patternCandidates.begin(); iter != patternCandidates.end(); ++iter) {
			indd patternDim = 0;
			vector<numm> allocatedTuple = (*iter); //->first;

			for (indd d = 0; d < D; ++d) {
				if (allocatedTuple[d] == allocatedTuple[D + d]) {++patternDim;}
			}
			if (patternDim < patternCutoff) {
				numm matchInSample = 0;
				for (indd s = 0; s < sample.size(); ++s) {
					if (matchNew(sample[s], allocatedTuple)) { matchInSample += 1; }
				}
				//iter->second[2 * O] /= (numm)matchInSample;
				//for (indd o = 0; o < O; ++o) {
				//	iter->second[2 * o] /= (numm)matchInSample;
				//	iter->second[2 * o + 1] /= (numm)matchInSample;
				//}
				numm maxGain = 0;

				// Insert new way of calculating GAIN here!!!
				numm gain = 0;
				numm indexTemp = 0;
				numm supportCheck = 0;

				// For all tuples t in dataset D,
				for (indd tupleIndex = 0; tupleIndex < tuples_new.size(); ++tupleIndex){
					if (matchNew(tuples_new[tupleIndex], allocatedTuple)) {
						supportCheck += 1;
						gain += normalizedMeasure[tupleIndex] * log(normalizedMeasure[tupleIndex]/normalizedMeasureEstimate[tupleIndex]);
					}
				}

				// END new way of calculating GAIN here!!!


				if (supportCheck / (numm) tuples_new.size() >= minSupport) {
					maxGain = max(maxGain, gain);
				}
				if (maxGain > minGain) {
					topTemplates.emplace_back(maxGain, allocatedTuple);
					if (topTemplates.size() > FLCandidates) {
						sort(topTemplates.rbegin(), topTemplates.rend());
						topTemplates.resize(FLCandidates);
						minGain = topTemplates[FLCandidates - 1].first;
					}
				}
			}
		}
		vector<numm> bestPattern(2 * D + 3, 0);
		numm bestGain = 0;
		for (auto iter = topTemplates.begin(); iter != topTemplates.end(); ++iter) {
			/////// BuildAdaptedCube -- Note that the cube is build compactely and efficiently, i.e., aggregation over non-constant attributes and the ranges occurs with the first pass of the data.
			// Find Thresholds
			vector<indd> dimIndices; // which dimensions is this cube build with
			vector<vector<indd>> dimThresholds; // thresholds for cumulative sum aggregration per dimenion
			vector<indd> realcenters;  // list of values per attribute that are the center of expansion
			vector<indd> transformedCenters; // as centers but as index of the aggregated buckets of the cumulative cube
			
			for (indd d = 0; d < D; ++d) {
				if (iter->second[d] == iter->second[D + d]) {
					indd center = iter->second[d];
					vector<indd> localThresholds = {center};
					indd smaller = center;
					for (indd steps = 1; smaller > 0; steps *= 2) {
						(smaller > steps) ? (smaller = smaller - steps) : (smaller = 0);
						localThresholds.push_back(smaller);
					}
					indd larger = center;
					for (indd steps = 1; larger < keyToAttribute[d].size() - 1; steps *= 2) {
						// MY STUFF???
						larger = min(larger + steps, (int)keyToAttribute[d].size() - 1);
						// larger = min(larger + steps, keyToAttribute[d].size() - 1);
						localThresholds.push_back(larger);
					}
					sort(localThresholds.begin(), localThresholds.end());
					dimIndices.push_back(d);
					dimThresholds.push_back(localThresholds);

					realcenters.push_back(center);
					transformedCenters.push_back(distance(localThresholds.begin(), find(localThresholds.begin(), localThresholds.end(), center)));
				}
			}
			indd patternDim = dimIndices.size();
			vector<numm> localBestPattern;
			// Build Cube
			// First pass on the data and consider which cell of the cumulative cube the tuples fall into, then accumulate values across dimensions
			// For efficiency, the high-dimensional cube (with variable dimension count) is built as one-dimensional vector using just multiplicative indexing
			indd cubeSize = 1;
			for (indd d = 0; d < patternDim; ++d) {
				cubeSize *= dimThresholds[d].size();
			}
			auto cumulativeCubeTrue = vector<vector<indd>>(O, vector<indd>(cubeSize, 0));
			auto cumulativeCubeModel = vector<vector<numm>>(O, vector<numm>(cubeSize, 0));
			auto cumulativeCubeSupport = vector<indd>(cubeSize, 0);
			for (auto iter = tuples.begin(); iter != tuples.end(); ++iter) {
				indd cubeID = 0;
				indd multiplier = 1;
				for (indd d = 0; d < patternDim; ++d) {
					indd value = iter->first[dimIndices[d]];
					indd bucket = 0;
					if (value < realcenters[d]) {
						while (value >= dimThresholds[d][bucket+1]) {++bucket;}
					} else {
						bucket = transformedCenters[d];
						while (value > dimThresholds[d][bucket]) { ++bucket; }
					}
					cubeID += multiplier*bucket;
					multiplier *= dimThresholds[d].size();
				}
				cumulativeCubeSupport[cubeID] += iter->second[O];
				for (indd o = 0; o < O; ++o) {
					cumulativeCubeModel[o][cubeID] += iter->second[O] * RCT.find(iter->second[O + 1])->second[o];
					cumulativeCubeTrue[o][cubeID] += iter->second[o];
				}
			}
			indd stepwidth = 1;
			for (indd d = 0; d < patternDim; ++d) {
				for (indd i = 0; i + stepwidth < cumulativeCubeSupport.size(); ++i) {
					indd target = i + stepwidth;
					if (target % (stepwidth * dimThresholds[d].size()) == 0) {
						i += (stepwidth - 1);
					} else {
						cumulativeCubeSupport[target] += cumulativeCubeSupport[i];
						for (indd o = 0; o < O; ++o) {
							cumulativeCubeModel[o][target] += cumulativeCubeModel[o][i];
							cumulativeCubeTrue[o][target] += cumulativeCubeTrue[o][i];
						}
					}
				}
				stepwidth *= dimThresholds[d].size();
			}
			// Greedy BFS Exploration of patterns
			// Note: patterns contain only those dimensions that are present in the cube (i.e. constants in the original, simple pattern)
			vector<numm> protoPattern(2*patternDim);
			for (indd d = 0; d < patternDim; ++d) {
				protoPattern[d] = transformedCenters[d];
				protoPattern[patternDim+d] = transformedCenters[d];
			}
			unordered_map<vector<numm>, numm, boost::hash<std::vector<numm>>> expandablePatterns;
			expandablePatterns.emplace(protoPattern, 0);
			while (expandablePatterns.size() > 0) {
				unordered_map<vector<numm>, numm, boost::hash<std::vector<numm>>> nextExpandablePatterns;
				for (auto iter = expandablePatterns.begin(); iter != expandablePatterns.end(); ++iter) {
					numm currGain = 0;
					vector<numm> currPattern = iter->first;
					//evaluate gain with cumulative cubes
					indd support = 0, supportN = 0;
					vector<indd> trueCount(O, 0), trueCountN(O, 0);
					vector<numm> modelCount(O, 0), modelCountN(O, 0);
					for (indd i = 0; i < pow(2, patternDim); ++i) {
						indd cubeID = 0;
						indd multiplier = 1;
						indd index = 0;
						bool valid = true;
						for (indd d = 0; d < patternDim; ++d) {
							if ((i >> d & 1) && (currPattern[d] == 0)) { valid = false; }
							else { ((i >> d & 1) ? (index += multiplier*(currPattern[d] - 1)) : (index += multiplier * currPattern[patternDim + d])); }
							multiplier *= dimThresholds[d].size();
						}
						if (valid) {
							// MY STUFF
							if ((_popcnt64(i) % 2) == 0) {   // __popcnt in Windows, __popcnt64 in 64-bit VC++
								support += cumulativeCubeSupport[index];
								for (indd o = 0; o < O; ++o) {
									trueCount[o] += cumulativeCubeTrue[o][index];
									modelCount[o] += cumulativeCubeModel[o][index];
								}
							}
							else {
								supportN += cumulativeCubeSupport[index];
								for (indd o = 0; o < O; ++o) {
									trueCountN[o] += cumulativeCubeTrue[o][index];
									modelCountN[o] += cumulativeCubeModel[o][index];
								}
							}
						}
					}

					// Need to do some gain stuff here...
					numm supportF = (support - supportN);

					// INSERT NEW WAY OF CALCULATING GAIN HERE!!!
					numm gain = 0;
					numm indexTemp = 0;
					numm supportCheck = 0;

					// For all tuples t in dataset D,
					for (indd tupleIndex = 0; tupleIndex < tuples_new.size(); ++tupleIndex){
						if (matchNew(tuples_new[tupleIndex], currPattern)) {
							support += 1;
							gain += normalizedMeasure[tupleIndex] * log(normalizedMeasure[tupleIndex]/normalizedMeasureEstimate[tupleIndex]);
						}
					}

					// END NEW WAY OF CALCULATING GAIN HERE!!!

					currGain = max(currGain, gain);
					if (gain > bestGain) {
						bestGain = gain;
						localBestPattern = currPattern;
						localBestPattern.push_back(0);
						localBestPattern.push_back(support - supportN);
						localBestPattern.push_back(trueCount[0] - trueCountN[0]);
					}

					if (currGain > iter->second) {
						for (indd d = 0; d < patternDim; ++d) {
							if (currPattern[d] > 0) {
								vector<numm> nextPattern = currPattern;
								nextPattern[d] -= 1;
								auto ref = nextExpandablePatterns.emplace(nextPattern, currGain);
							}
							if (currPattern[patternDim+d] < dimThresholds[d].size()-1) {
								vector<numm> nextPattern = currPattern;
								nextPattern[patternDim +d] += 1;
								auto ref = nextExpandablePatterns.emplace(nextPattern, currGain);
							}
						}
					}
				}
				expandablePatterns = nextExpandablePatterns;
			}
			// construct best pattern from the small pattern, i.e. fill range of remaining attributes with wildcards
			if (localBestPattern.size() > 0) {
				for (indd d = 0; d < D; ++d) {
					bestPattern[d] = 0;
					bestPattern[D + d] = keyToAttribute[d].size() - 1;
				}
				for (indd d = 0; d < patternDim; ++d) {
					bestPattern[dimIndices[d]] = dimThresholds[d][localBestPattern[d]];
					bestPattern[D + dimIndices[d]] = dimThresholds[d][localBestPattern[patternDim + d]];
				}
				for (indd extras = 0; extras < 3; ++extras) { bestPattern[2*D+extras] =  localBestPattern[2*patternDim+extras]; }
			}
		}
		// insert best pattern into the explanation table
		addRuleNewDebug(bestPattern); //addRuleNewDebug
		
		iterativeScalingNewRCT(); //iterativeScaling();
		//iterativeScaling(); //(Until bug fixed????)

		auto end = chrono::high_resolution_clock::now();
		if (verbose) {
			numm currentKL = KLDivergenceNew();
			cout << "FL+OptGrow " << samplesize << ";" <<FLCandidates<<"," << rules + 1 << "," << chrono::duration_cast<chrono::milliseconds>(end - start).count() << "," << chrono::duration_cast<chrono::milliseconds>(end - globalStart).count() << ",";
			cout << recentKL - currentKL << "," << baseKL - currentKL << "," << currentKL << "\n";
			recentKL = currentKL;
		}
	}
}
