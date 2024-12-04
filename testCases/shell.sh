echo "HELLO1"
echo "This is a test file" > input.txt
echo "HELLO2"
touch empty.txt
echo "Old content" > output.txt
echo "=== Redirection Tests ==="
echo "[Test 1] Input redirection with a valid file:"
cat < input.txt
echo "[Test 2] Input redirection with a nonexistent file:"
cat < non_existent_file.txt
echo "[Test 3] Input redirection with an empty file:"
cat < empty.txt
echo "[Test 4] Output redirection to a new file:"
echo Hello World > new_output.txt
cat new_output.txt
echo "[Test 5] Output redirection overwriting an existing file:"
echo 'Overwritten content' > output.txt
cat output.txt
echo "[Test 7] Input and output redirection with valid files:"
cat < input.txt > new_output.txt
cat new_output.txt
echo "[Test 8] Input redirection with missing input file:"
cat < missing_file.txt > output.txt
echo "=== Redirection Tests Completed ==="
rm -f input.txt output.txt new_output.txt empty.txt
exit