#include <fstream>

#include <gtest/gtest.h>

#include "Fixtures.h"

#include "query.h"
#include "utils.h"

using namespace std;
using namespace SpiceQL;


TEST(QueryTests, UnitTestGetLatestKernel) {
  vector<string> kernels = {
    "iak.0001.ti",
    "iak.0003.ti",
    "different/place/iak.0002.ti",
    "test/iak.0004.ti"
  };

  EXPECT_EQ(getLatestKernel(kernels),  "test/iak.0004.ti");
}

TEST(QueryTests, getKernelStringValue){
  unique_ptr<Kernel> k(new Kernel("data/msgr_mdis_v010.ti"));
  // INS-236810_CCD_CENTER        =  (  511.5, 511.5 )
  EXPECT_EQ(getKernelStringValue("INS-236810_FOV_SHAPE"), "RECTANGLE");


     try {
        getKernelStringValue("aKeyThatWillNotBeInTheResults");
        FAIL() << "Expected std::invalid_argument";
    }
    catch(std::invalid_argument const & err) {
        EXPECT_EQ(err.what(),std::string("key not in results"));
    }
    catch(...) {
        FAIL() << "Expected std::invalid_argument";
    }
}

TEST(QueryTests, getKernelVectorValue){
  unique_ptr<Kernel> k(new Kernel("data/msgr_mdis_v010.ti"));

  vector<string> actualResultsOne = getKernelVectorValue("INS-236810_CCD_CENTER");
  std::vector<string> expectedResultsOne{"511.5", "511.5"};

  vector<string> actualResultsTwo = getKernelVectorValue("INS-236800_FOV_REF_VECTOR");
  std::vector<string> expectedResultsTwo{"1.0", "0.0", "0.0"};

  for (int i = 0; i < actualResultsOne.size(); ++i) {
    EXPECT_EQ(actualResultsOne[i], expectedResultsOne[i]) << "Vectors x and y differ at index " << i;
  }

  for (int j = 0; j < actualResultsTwo.size(); ++j) {
    EXPECT_EQ(actualResultsTwo[j], expectedResultsTwo[j]) << "Vectors x and y differ at index " << j;
  }

   try {
        getKernelVectorValue("aKeyThatWillNotBeInTheResults");
        FAIL() << "Expected std::invalid_argument";
    }
    catch(std::invalid_argument const & err) {
        EXPECT_EQ(err.what(),std::string("key not in results"));
    }
    catch(...) {
        FAIL() << "Expected std::invalid_argument";
    }
}

TEST(QueryTests, UnitTestGetLatestKernelError) {
  vector<string> kernels = {
    "iak.0001.ti",
    "iak.0003.ti",
    "different/place/iak.0002.ti",
    "test/iak.4.ti",
    // different extension means different filetype and therefore error
    "test/error.tf"
  };

  try {
    getLatestKernel(kernels);
    FAIL() << "expected invalid argument error";
  }
  catch(invalid_argument &e) {
    SUCCEED();
  }
}


TEST_F(KernelDataDirectories, FunctionalTestSearchMissionKernelsAllMess) {
  string dbPath = getMissionConfigFile("mess");

  ifstream i(dbPath);
  nlohmann::json conf;
  i >> conf;

  MockRepository mocks;
  mocks.OnCallFunc(ls).Return(paths);

  nlohmann::json res = searchMissionKernels("/isis_data/", conf);

  ASSERT_EQ(res["mdis"]["ck"]["reconstructed"]["kernels"].size(), 4);
  ASSERT_EQ(res["mdis"]["ck"]["smithed"]["kernels"].size(), 4);
  ASSERT_EQ(res["mdis"]["ck"]["deps"]["objs"].size(), 4);
  ASSERT_EQ(res["mdis"]["spk"]["reconstructed"]["kernels"].size(), 2);
  ASSERT_EQ(res["mdis"]["tspk"]["kernels"].size(), 1);
  ASSERT_EQ(res["mdis"]["fk"]["kernels"].size(), 2);
  ASSERT_EQ(res["mdis"]["ik"]["kernels"].size(), 2);
  ASSERT_EQ(res["mdis"]["iak"]["kernels"].size(), 2);
  ASSERT_EQ(res["mdis"]["pck"]["na"]["kernels"].size(), 2);

  ASSERT_EQ(res["mdis_att"]["ck"]["reconstructed"]["kernels"].size(), 4);
  ASSERT_EQ(res["mdis_att"]["ck"]["deps"]["objs"].size(), 2);

  ASSERT_EQ(res["mess"]["ck"]["reconstructed"]["kernels"].size(), 5);
  ASSERT_EQ(res["mess"]["sclk"]["kernels"].size(), 2);
  ASSERT_EQ(res["mess"]["ck"]["deps"]["objs"].size(), 2);
}


TEST_F(KernelDataDirectories, FunctionalTestSearchMissionKernelsClem1) {
  fs::path dbPath = getMissionConfigFile("clem1");

  ifstream i(dbPath);
  nlohmann::json conf;
  i >> conf;

  MockRepository mocks;
  mocks.OnCallFunc(ls).Return(paths);

  nlohmann::json res = searchMissionKernels("/isis_data/", conf);

  ASSERT_EQ(res["clem1"]["ck"]["reconstructed"]["kernels"].size(), 4);
  ASSERT_EQ(res["clem1"]["ck"]["smithed"]["kernels"].size(), 1);
  ASSERT_EQ(res["clem1"]["ck"]["deps"]["objs"].size(), 2);
  ASSERT_EQ(res["clem1"]["spk"]["reconstructed"]["kernels"].size(), 2);
  ASSERT_EQ(res["clem1"]["fk"]["kernels"].size(), 1);
  ASSERT_EQ(res["clem1"]["sclk"]["kernels"].size(), 2);
  
  ASSERT_EQ(res["uvvis"]["ik"]["kernels"].size(), 1);
  ASSERT_EQ(res["uvvis"]["iak"]["kernels"].size(), 2);

  ASSERT_EQ(res["uvvis"]["iak"]["kernels"].size(), 2);
}


TEST_F(KernelDataDirectories, FunctionalTestSearchMissionKernelsGalileo) {
  fs::path dbPath = getMissionConfigFile("galileo");

  ifstream i(dbPath);
  nlohmann::json conf;
  i >> conf;

  MockRepository mocks;
  mocks.OnCallFunc(ls).Return(paths);

  nlohmann::json res = searchMissionKernels("/isis_data/", conf);

  ASSERT_EQ(res["galileo"]["ck"]["reconstructed"]["kernels"].size(), 4);
  ASSERT_EQ(res["galileo"]["ck"]["reconstructed"]["deps"].size(), 0);
  ASSERT_EQ(res["galileo"]["ck"]["smithed"]["kernels"].size(), 3);
  ASSERT_EQ(res["galileo"]["ck"]["smithed"]["deps"]["objs"].size(), 1);
  ASSERT_EQ(res["galileo"]["ck"]["deps"]["objs"].size(), 2);
  ASSERT_EQ(res["galileo"]["spk"]["reconstructed"]["kernels"].size(), 2);
  ASSERT_EQ(res["galileo"]["iak"]["kernels"].size(), 1);
  ASSERT_EQ(res["galileo"]["pck"]["smithed"]["kernels"].size(), 2);
  ASSERT_EQ(res["galileo"]["pck"]["smithed"]["deps"].size(), 0);
  ASSERT_EQ(res["galileo"]["pck"]["na"]["kernels"].size(), 1);
  ASSERT_EQ(res["galileo"]["pck"]["na"]["deps"].size(), 0);
  ASSERT_EQ(res["galileo"]["sclk"]["kernels"].size(), 1);
}

