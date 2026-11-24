# Smart Lost & Found System

A Data Structures project in C that stores lost and found reports, automatically matches them, and gives the shortest campus route to where a found item is kept.

## Team
- Saurabh Singh Shahi (04896303125)
- Ankush Kumar Raj (05296303125)
- Aryaman Yadav (02896302825)

## How to run
```
gcc -std=c99 -Wall lost_and_found.c -o lostfound
./lostfound
```
## Features
- Report lost and found items
- Automatic matching with a score
- Sorted listing by name, date or priority
- Search by ID and by name
- Claim queue and undo
- Shortest route to the item (Dijkstra)

## Data structures used
- Hash table with chaining: lookup by item ID
- Linked list: items per category
- Binary search tree: items sorted by name
- Max-heap: ranking the best matches
- Queue: claim requests, first come first served
- Stack: undo
- Merge sort, sequential search and binary search
- Graph with Dijkstra's algorithm: campus directions
