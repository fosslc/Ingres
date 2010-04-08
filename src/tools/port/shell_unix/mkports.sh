:
#!/bin/sh
# compact a path into a porting structure for source
cd $1
mv * ../..
cd ..
rmdir src
cd ..
rmdir $ING_VERS
cd ../..
