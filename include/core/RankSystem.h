#pragma once
#include <string>
#include <vector>
#include <map>

struct Rank {
    std::string yourRank;        
    double requiredWeight;     
};

class RankSystem {
public:
    RankSystem();
    std::string getRank(const std::string& exerciseName, double weight) const;
    Rank getNextRank(const std::string& exerciseName, double weight) const;
    double weightToNextRank(const std::string& exerciseName, double weight) const;

    bool hasExercise(const std::string& exerciseName) const;

private:
    std::map<std::string, std::vector<Rank>> ranks;

    void initRanks();
};