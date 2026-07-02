# Travel Planner AVL

A console-based **Travel Planner System** developed in **C** that efficiently manages travel itineraries using **AVL Trees**. The project supports chronological activity scheduling, navigation route management, hotel booking details, path searching, range queries, and several travel-related operations.

---

## Features

- Create and manage complete travel itineraries
- Chronological activity scheduling using AVL Trees
- Flight, Hotel, Tourist Place, and Transport activities
- Navigation route management for every activity
- Add, Update, Delete, and Search navigation directions
- Range Search between dates
- Source → Destination path finder
- Detect duplicate visited locations
- Sort activities by hotel charges
- Date & Time validation
- Dynamic memory management

---

## Data Structures Used

- AVL Tree
- Nested AVL Tree
- Dynamic Memory Allocation
- Structures
- Enumerations

---

## Algorithms Used

### Trip Management AVL Tree

Stores travel activities in chronological order.

Supports:

- Insertion
- Deletion
- Searching
- Automatic balancing

---

### Navigation AVL Tree

Each activity contains its own AVL Tree for navigation directions.

Supports:

- Add direction
- Insert direction
- Delete direction
- Update direction
- Search directions

---

### Additional Algorithms

- AVL Rotations
- Range Search
- Chronological Traversal
- Duplicate Detection
- Hotel Cost Sorting (using qsort)
- Path Search
- Date Validation

---

## Activity Types

- Flight
- Hotel Stay
- Tourist Visit
- Transport

---

## Menu Operations

### Trip Operations

- Create Trip
- Add Activity
- Insert Activity
- Delete Activity
- Display Trip

### Navigation Operations

- Add Direction
- Insert Direction
- Delete Direction
- Update Direction
- Search Direction
- Display Navigation Route

### Other Features

- Path Finder
- Duplicate Location Detection
- Sort by Hotel Charges
- Range Search

---

## Project Structure

```text
Travel-Planner-AVL/
│
├── travel_planner.c
└── README.md
```

---

## Example Workflow

```text
Create Trip

↓

Add Flight

↓

Add Hotel

↓

Add Tourist Place

↓

Add Navigation Directions

↓

Display Complete Itinerary

↓

Search Routes

↓

Find Paths

↓

Sort Hotel Charges

↓

Range Search
```

---

## Concepts Demonstrated

- AVL Trees
- Self-Balancing Trees
- Nested Data Structures
- Dynamic Memory Allocation
- Tree Traversals
- Searching Algorithms
- Sorting
- Date-Time Validation
- Travel Itinerary Management

---

## Technologies

- C
- Standard C Library

---

## Future Improvements

- Graph-based shortest path (Dijkstra)
- File storage support
- GUI version
- Interactive map integration
- Expense tracking
- Multi-user support

---

## Learning Outcome

This project demonstrates the practical use of AVL Trees in building an efficient travel itinerary management system. It combines self-balancing trees, dynamic memory management, searching, sorting, and hierarchical data structures to efficiently organize and retrieve travel information.