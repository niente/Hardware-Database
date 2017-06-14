# Hardware-Database
This program emulates a hardware database which is stored in a local binary file. Records are read/written by hashing to this file.
Input is validated using various C string functions.

Sample output:
```
Deleting old output.txt
Opening input file: input.txt

Opening output file: output.txt

Insert: Record 6745 added to bucket 4.
Insert: Record 5675 added to bucket 33.
Insert: Record 1235 added to bucket 17.
Insert: Record 2341 added to bucket 28.
Insert: Record 8624 added to bucket 8.
Insert: Record 9162 added to bucket 26.
Insert: Record 7146 added to bucket 16.
Insert: Record 2358 added to bucket 24.
Insert: Record 1622 added to bucket 33.
Insert: Record 1832 added to bucket 36.
Insert: Record 3271 added to bucket 35.
Insert: Record 4717 added to bucket 7.
Insert: Record 9524 added to bucket 38.
Insert: Record 1524 added to bucket 14.
Insert: Record 5219 added to bucket 39.
Insert: Record 6275 added to bucket 36.
Insert: Record 5392 added to bucket 1.
Insert: Record 5192 added to bucket 39.

To search the item database, press 1.
To insert from standard input, press 2.
To insert from a file, press 3.
To delete a record, press 4.
To quit, press Q.
4
Enter the ID of a record you want to delete, or Q to quit.
5192
Deleting record:
5192 SCREW DRIVER 789

To search the item database, press 1.
To insert from standard input, press 2.
To insert from a file, press 3.
To delete a record, press 4.
To quit, press Q.
2
To insert an item, please enter a line of text in the following format:
####,ITEM NAME:##
(ID),         :Quantity
Please enter a line to parse, or type Q to quit:
11111,Test Item:0
ID must be 4 digits! Unable to read 11111
Exiting.
Please enter a line to parse, or type Q to quit:
4717, Duplicate Test:100
Duplicate ID detected! Unable to insert  DUPLICATE TEST.
Please enter a line to parse, or type Q to quit:
abcd,Test Item:100
ID must be 4 digits! Unable to read abcd
Exiting.
Please enter a line to parse, or type Q to quit:
1111, Long Item Name (very long):100
Name cannot be longer than 20 characters! Unable to read  LONG ITEM NAME (VERY LONG)
Exiting.
```
