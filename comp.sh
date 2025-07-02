cd /home/arno/Code/Pheno/PhenoLink
cmake -S . -B build -G "Unix Makefiles" \
  -DCMAKE_C_COMPILER=/usr/bin/gcc \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
  -DCMAKE_MAKE_PROGRAM=/usr/bin/make \
  -DCMAKE_TOOLCHAIN_FILE=/home/arno/Code/Pheno/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DVCPKG_INSTALL_OPTIONS=--x-buildtrees-root=/home/arno/Code/Pheno/vcpkg/buildtrees/ \
  -DQt6_DIR=/home/arno/Qt/6.5.3/gcc_64/lib/cmake/Qt6 \
  -DICU_INCLUDE_DIR=/home/arno/Code/Pheno/icu/usr/local/include/ \
  -DICU_UC_LIBRARY=/home/arno/Code/Pheno/icu/usr/local/lib/libicuuc.so \
  -DICU_DATA_LIBRARY=/home/arno/Code/Pheno/icu/usr/local/lib/libicudata.so \
  -DICU_I18N_LIBRARY=/home/arno/Code/Pheno/icu/usr/local/lib/libicui18n.so \
  -DJxl_INCLUDE_DIRS=/usr/local/include/jxl/ \
  -DJxl_LIBRARY_DIRS=/usr/local/lib/ \
  -DQT_DEBUG_FIND_PACKAGE=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=1
