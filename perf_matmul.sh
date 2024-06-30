rm -f temp.csv
touch temp.csv

echo "m,time" > temp.csv

num_tiles=(20 40 60 80 100)

for m in "${num_tiles[@]}"
do
    ./build/matmul --m=$m --n=$m --k=$m >> temp.csv
done

rm -f matmul.csv
mv temp.csv matmul.csv
