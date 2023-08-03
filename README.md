# SpiceQL
[![Documentation Status](https://readthedocs.org/projects/sugar-spice/badge/?version=latest)](http://sugar-spice.readthedocs.io/?badge=latest) [![CMake](https://github.com/USGS-Astrogeology/SpiceQL/actions/workflows/ctests.yml/badge.svg)](https://github.com/USGS-Astrogeology/SpiceQL/actions/workflows/ctests.yml)

This Library provides a C++ interface querying, reading and writing Naif SPICE kernels. Built on the [Naif Toolkit](https://naif.jpl.nasa.gov/naif/toolkit.html).


## Building The Library

The library leverages anaconda to maintain all of it's dependencies. So in order to build SpiceQL, you'll need to have Anaconda installed.

> **NOTE**:If you already have Anaconda installed, skip to step 3.

1. Download either the Anaconda or Miniconda installation script for your OS platform. Anaconda is a much larger distribtion of packages supporting scientific python, while Miniconda is a minimal installation and not as large: Anaconda installer, Miniconda installer
1. If you are running on some variant of Linux, open a terminal window in the directory where you downloaded the script, and run the following commands. In this example, we chose to do a full install of Anaconda, and our OS is Linux-based. Your file name may be different depending on your environment.
   * If you are running Mac OS X, a pkg file (which looks similar to Anaconda3-5.3.0-MacOSX-x86_64.pkg) will be downloaded. Double-click on the file to start the installation process.
1. Open a Command line prompt and run the following commands:

```bash
# Clone the Github repo, note the recursive flag, this library depends on
# submodules that also need to be cloned. --recurse-submodules enables this and
# the -j8 flag parallelizes the cloning process.
git clone --recurse-submodules -j8 https://github.com/USGS-Astrogeology/SpiceQL.git

# cd into repo dir
cd SpiceQL

# Create new environment from the provided dependency file, the -n flag is
# proceded by the name of the new environment, change this to whatever works for you
conda env create -f environment.yml -n ssdev

# activate the new env
conda activate ssdev

# make and cd into the build directory. This can be placed anywhere, but here, we make
# it in the repo (build is in .gitingore, so no issues there)
mkdir build
cd build

# Configure the project, install directory can be anything, here, it's the conda env
cmake .. -DCMAKE_INSTALL_PREFIX=$CONDA_PREFIX

# Optional: DB files are installed by default in $CONDA_PREFIX/etc/SpiceQL/db to 
# use files that are included within the repo, you must create and define 
# an environment variable named SSPICE_DEBUG. 
# note SSPICE_DEBUG can be set to anything as long as it is defined
export SSPICE_DEBUG=True

# Set the environment variable(s) to point to your kernel install 
# The following environment variables are used by default in order of priority: 
# $SPICEROOT, $ALESPICEROOT, $ISISDATA. 
# SPICEROOT is unique to this lib, while ALESPICEROOT, and ISISDATA are used 
# by both ALE and ISIS respectively. 
# note you can set each of these environment variables path to point to the
# correspoding kernels downloaded location, ie 
SPICEROOT=~/spiceQL/Kernals/spiceRootKernel
ALESPICEROOT=~/spiceQL/Kernals/aleSpiceRootKernel
ISISDATA=~/spiceQL/Kernals/isisData

# build and install project
make install

# Optional, Run tests
ctest -j8
```

You can disable different components of the build by setting the CMAKE variables `SPICEQL_BUILD_DOCS`, `SPICEQL_BUILD_TESTS`, `SPICEQL_BUILD_BINDINGS`, or `SPICEQL_BUILD_LIB` to `OFF`. For example, the following cmake configuration command will not build the documentation or the tests:

```
cmake .. -DCMAKE_INSTALL_PREFIX=$CONDA_PREFIX -DSPICEQL_BUILD_DOCS=OFF -DSPICEQL_BUILD_TESTS=OFF
```

## Bindings

The SpiceQL API is available via Python bindings in the module `pyspiceql`. The bindings are built using SWIG and are on by default. You can disable the bindings in your build by setting `SPICEQL_BUILD_BINDINGS` to `OFF` when configuring your build.

## Memoization Header Library 

SpiceQL has a simple memoization header only library at `Spiceql/include/memo.h`. This can cache function results on disk using a binary archive format mapped using a combined hash of a function ID and it's input parameters. 

TLDR 
```C++
#include "memo.h"

int func(int) { ... }
memoization::disk c("cache_path");

// use case 1: wrap function call
// (function ID, the function to wrap and then params
int result1 = c("func_id", func, 3);

// use case 2: wrap function
// (cache object, function ID, function)
auto func_memoed = memoization::make_memoized(c, "func_id", func);
int result2 = func_memoed(3);

assert(result1 == result2);
```


## Building and Testing Lambda Locally

1. Use docker compose to run the container: `docker compose up -d --build`

2. Post an event to the following endpoint using a `curl` command:
`curl -XGET "http://localhost:9000/2015-03-31/functions/function/invocations" -d '{"func" : "<FunctionName>", "arg1" : "<FirstArgument>", "arg2": "<SecondArgument>", "arg3": "<ThirdArgument>"}'`

## Interacting with Public(VPN) AWS Lambda Function

### Production API
See [openapi-prod.yml](https://code.chs.usgs.gov/asc/SpiceQL/-/blob/main/aws/openapi-prod.yml) for documentation on querying the API.

### Dev API
See [openapi-prod.yml](https://code.chs.usgs.gov/asc/SpiceQL/-/blob/main/aws/openapi.yml) for documentation on querying the API.


## Devs Adding new Endpoints to API

To expose a new function as an endpoint to the API:

1. Create a new lambda function in [aws/lambda_function.py](https://code.chs.usgs.gov/asc/SpiceQL/-/blob/main/aws/lambda_function.py) that calls the pyspiceql wrapper function associated with the function you wish to expose.

2. Add a new endpoint path to [aws/openapi.yml](https://code.chs.usgs.gov/asc/SpiceQL/-/blob/main/aws/openapi.yml) with associated function parameters. 

   For uri: `"arn:aws:apigateway:us-west-2:lambda:path/2015-03-31/functions/arn:aws:lambda:us-west-2:950438895271:function:<functionName>lambda/invocations"`

3. Add new `AWS::Lambda::Function` and `AWS::Lambda::Permission` resources to [aws/SpiceQLambdaStack.yml](https://code.chs.usgs.gov/asc/SpiceQL/-/blob/main/aws/SpiceQLambdaStack.yml).

   Be sure that the function name matches the one added in the uri of the openapi.yml.

   Update the ImageConfig command to match the function name in lambda_function.py: `Command: ["lambda_function.lambda_<functionName>"]`

   In the `AWS::Lambda::Permission` resource, set the SourceArn specific to the endpoint you set up in openapi.yml. 

5. Add lambda function name to "functions" list in the [gilab-ci.yml](https://code.chs.usgs.gov/asc/SpiceQL/-/blob/main/.gitlab-ci.yml#L159).

6. Once these updated files are merged into the main branch, The CI will automatically update the Cloudformation and API Gateway to include the new endpoints. 


## Deploy to Production

1. Create a repo release and tick the version number according to [semver.org](https://semver.org). 

2. Deploy new image to ECR

   `aws ecr get-login-password --region us-west-2 | docker login --username AWS --password-stdin 950438895271.dkr.ecr.us-west-2.amazonaws.com`

   `git submodule update --init --recursive`

   `docker build -t spiceql .`

   `docker tag spiceql:latest 950438895271.dkr.ecr.us-west-2.amazonaws.com/spiceql:<new-version-tick>`

   `docker push 950438895271.dkr.ecr.us-west-2.amazonaws.com/spiceql:<new-version-tick>`

3. Update the StageName, Stage, and ApiMappingKey in [SpiceQlMainStack-Prod.yml](https://code.chs.usgs.gov/asc/SpiceQL/-/blob/main/aws/SpiceQlMainStack-Prod.yml) to `v<new-version-tick(no dots)>`. Make sure to only enter the numbers. API Gateway cannot handle symbols in the versioning.

4. Update the version and URL in [openapi-prod.yml](https://code.chs.usgs.gov/asc/SpiceQL/-/blob/main/aws/openapi-prod.yml) with new version tick.

5. If any lambda functions are added to `SpiceQLambdaStack.yaml` or `openapi.yaml` in the new release, copy and paste them into `SpiceQLambdaStack-Prod.yaml` and `openapi-prod.yaml`. Update the images to the new version tick for each lambda function. If new functions are added to `SpiceQlamdbdaStack-Prod.yaml`, make sure to add "-prod" after the existing function name (e.g. strSclkToEtlambda -> strSclkToEtlambda-prod).

6. Once all changes have been made, submit a merge request.

7. Update API in AWS

   `aws s3 cp aws/openapi-prod.yml s3://spiceql-openapi-files`

   `aws apigateway put-rest-api --rest-api-id	roiq1c4q37 --region us-west-2 --mode overwrite --body 'fileb://aws/openapi-prod.yml'`

   `aws apigateway create-deployment --rest-api-id roiq1c4q37 --region us-west-2 --stage-name v<new-version-tick(no dots)>`

8.  Update Cloudformation Stacks
    
    `aws s3 cp aws/SpiceQLambdaStac-Prod.yml s3://cf-templates-1bxumkby1rqpi-us-west-2/spiceql-cf-templates/`

    `aws s3 cp aws/SpiceQlMainStack-Prod.yml s3://cf-templates-1bxumkby1rqpi-us-west-2/spiceql-cf-templates/`

    `aws cloudformation update-stack --region us-west-2 --stack-name spiceql-prod --template-url https://s3.amazonaws.com/cf-templates-1bxumkby1rqpi-us-west-2/spiceql-cf-templates/SpiceQlMainStack-Prod.yml`

9. Update Lambda Function Images

   For each lambda function run:
   `aws lambda update-function-code --region us-west-2 --function-name <function-name> --image-uri 950438895271.dkr.ecr.us-west-2.amazonaws.com/spiceql:<new-version-tick(with dots)>`













