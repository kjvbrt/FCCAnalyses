#include "FCCAnalyses/ReconstructedParticle.h"

// Catch2
#include "catch2/catch_test_macros.hpp"
#include <catch2/catch_approx.hpp>

TEST_CASE("sel_pdg", "[ReconstructedParticle]") {
  ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> pVec;
  edm4hep::ReconstructedParticleData p1;
  p1.PDG = 11;
  pVec.push_back(p1);
  edm4hep::ReconstructedParticleData p2;
  p2.PDG = 13;
  pVec.push_back(p2);
  edm4hep::ReconstructedParticleData p3;
  p3.PDG = -11;
  pVec.push_back(p3);
  edm4hep::ReconstructedParticleData p4;
  p4.PDG = -13;
  pVec.push_back(p4);
  FCCAnalyses::ReconstructedParticle::sel_pdg selPdg{11};
  auto res = selPdg(pVec);
  REQUIRE(res.size() == 1);
  REQUIRE(res[0].PDG == 11);
}

TEST_CASE("sel_absPdg", "[ReconstructedParticle]") {
  ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> pVec;
  edm4hep::ReconstructedParticleData p1;
  p1.PDG = 11;
  pVec.push_back(p1);
  edm4hep::ReconstructedParticleData p2;
  p2.PDG = 13;
  pVec.push_back(p2);
  edm4hep::ReconstructedParticleData p3;
  p3.PDG = -11;
  pVec.push_back(p3);
  edm4hep::ReconstructedParticleData p4;
  p4.PDG = -13;
  pVec.push_back(p4);
  FCCAnalyses::ReconstructedParticle::sel_absPdg selAbsPdg{11};
  auto res = selAbsPdg(pVec);
  REQUIRE(res.size() == 2);
  REQUIRE(res[0].PDG == 11);
  REQUIRE(res[1].PDG == -11);
}

TEST_CASE("sel_absPdg__neg_pdg", "[ReconstructedParticle]") {
  REQUIRE_THROWS_AS(FCCAnalyses::ReconstructedParticle::sel_absPdg(-17),
                    std::invalid_argument);
}
