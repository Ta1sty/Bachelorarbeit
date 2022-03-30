@echo off
echo DOWNLOADING MOANA ISLAND
curl --output island.tgz --url https://wdas-datasets-disneyanimation-com.s3-us-west-2.amazonaws.com/moanaislandscene/island-pbrt-v1.1.tgz
echo DOWNLOAD FINISHED
echo EXTRACTING THIS CAN TAKE A WHILE
tar -xf island.tgz
echo EXTRACT FINISHED
echo DELETING TGZ
del island.tgz
PAUSE