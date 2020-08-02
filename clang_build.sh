if [ ! -d "build" ]; then
    mkdir build
fi;

echo "# Building using clang..."
clang recipes.c -std=c99 -o build/recipes

if [ $? == 0 ]; then
    echo "# Running build..."
    build/recipes
fi;
