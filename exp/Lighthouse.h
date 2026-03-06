#ifndef LIGHTHOUSE_H
#define LIGHTHOUSE_H

#include "Includes.h"
#include <map>
#include "StaticFunctions.h"
#include <boost/dynamic_bitset.hpp>
#include <boost/container_hash/hash.hpp>

class Lighthouse
{
	struct tupleT
	{
    		boost::dynamic_bitset<> bitArray;
    		numm measure;
    		numm estimate;
	};
	vector<tupleT> tuplesVectorNew;
	unordered_map<boost::dynamic_bitset<>, vector<numm>> RCT_New; // bitset -> count, sumMeasure, sumEstimate

	// Vector for just tuple values
	vector<vector<numm> > tuples_new; // [attributes]

	vector<string> attributeNames;

	unordered_map<indd, string> keyToOutcome;
	unordered_map<string, indd> outcomeToKey;
	vector<unordered_map<indd, numm>> keyToAttribute;
	vector<unordered_map<numm, indd>> attributeToKey;

	unordered_map<vector<indd>, vector<indd>, ind_vector_hasher> tuples; // [Attributes] -> [outcomes1, outcome2, ..., totalcount, RCTIndex]

	indd D;
	indd N;
	indd O;
	indd totalN;

	indd rulesCount;

	vector<numm> lambdas; // R_j -> Lambda_j
	unordered_map<indd, vector<numm>> RCT; // region -> [models, rulescounts per outcome, tuplecounts per outcome, total Tuples Counts]

	vector<numm> lambdasNew; // R_j -> Lambda_j
	vector<numm> ruleMeasures; // R_j -> Measure_j
        vector<numm> ruleEstimates; // R_j -> Measure_estimate

	vector<vector<numm>> patterns; // [Attributes, outcome, support, correctCount]
	bool match(vector<indd>& tuple, vector<indd>& pattern);
	bool match(vector<numm>& tuple, vector<indd>& pattern);
	bool matchNew(vector<numm>& tuple, vector<numm>& pattern);

	bool patternsEqual(vector<indd>& pattern1, vector<indd>& pattern2);
	bool patternsEqualNew(vector<numm>& pattern1, vector<numm>& pattern2);

	void addRule(vector<indd>& rule);
	void addRuleNew(vector<indd>& rule);
	void addRuleNewDebug(vector<numm>& rule);

	void iterativeScaling();
	void iterativeScalingNew();
	void iterativeScalingNewRCT();

	numm evalStraight(vector<indd>& pattern, vector<indd>& bestPattern, numm& bestGain);
	void generateSample(vector<vector<indd>>& sample, indd sampleSize);

	void generateSampleNew(vector<vector<numm>>& sample, indd sampleSize);


	void addSimpleAncestors(vector<indd>& protoPattern, unordered_map<vector<indd>, vector<numm>, ind_vector_hasher>& candidates, indd depth, vector<indd>& data);

	void addSimpleAncestorsNew(vector<numm>& protoPattern, unordered_set<vector<numm>, boost::hash<std::vector<numm>>>& candidates, indd depth);


public:
	Lighthouse(string filename, vector<indd> dataColumns, indd outcomeColumn, indd maxRules);
	~Lighthouse();

	numm KLDivergence();
	numm KLDivergenceNew();

	bool originalFlashlight(indd numberRules, indd sampleSize, numm minSupport, bool removeInactivatedPatterns, bool verbose);

	void printTable(numm baseKL, string fileName);
	string getTable();
	void LENS(indd numberRules, bool verbose, indd sampleSize, indd FLCandidates, indd mode, numm minSupport);
};
#endif // !LIGHTHOUSE_H