:
#!/bin/sh
# compact a path into a porting structure for headers
cd $1
cd ..
mv * ..
cd ..
rmdir $ING_VERS
cd ../..
