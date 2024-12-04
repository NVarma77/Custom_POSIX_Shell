
echo "Running redirection tests..."


echo cat < input.txt
cat < input.txt

echo Hello > output.txt
Hello > output.txt
cat output.txt


echo grep Hello < input.txt > output_filtered.txt
grep Hello < input.txt > output_filtered.txt
cat output_filtered.txt

echo "Redirection tests completed."
