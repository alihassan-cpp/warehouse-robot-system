# Warehouse Robot System

A console-based warehouse robot and order fulfillment system written in C++. It manages a warehouse layout, product stock, customer orders and a fleet of robots, then plans and runs the picking trips.

Every data structure is written by hand. The project uses no STL containers or algorithms (no `vector`, `map`, `queue`, `stack`, `sort` and so on).

## What it does

The program opens with eight menus, one per module.

| # | Module | What you can do |
|---|--------|-----------------|
| 1 | Warehouse layout and pathways | View locations, add locations and pathways, block and reopen pathways, check if two locations are connected (BFS), find the shortest route (Dijkstra) |
| 2 | Product inventory and racks | Insert, delete and search products, manage rack entries, update quantities, see total and available stock, low-stock report, preview rack allocation |
| 3 | Customer orders | Add orders, add products to an order, queue pending orders (FIFO), cancel orders |
| 4 | Robot fleet | Register robots, check eligibility for an order, assign and release robots, send robots to maintenance |
| 5 | Robot assignment and picking plans | Build and preview a picking plan for an order, reserve stock, view all assigned plans |
| 6 | Order picking and delivery | Execute a picking task for an assigned order |
| 7 | Battery and charging | Find robots that need charging, find a reachable charging station, charge or queue a robot |
| 8 | Operation history and undo | Undo manual quantity updates, blocked pathways and maintenance changes |

## Data structures

- **AVL tree** keeps products sorted by ID. Insert and delete rebalance with rotations, so search stays O(log n). There is a menu option that draws the tree so you can see the balance.
- **Weighted undirected graph** (adjacency lists) models the warehouse. BFS checks connectivity over open pathways, and Dijkstra finds the shortest route.
- **Queue** holds pending orders (first come, first served) and robots waiting for a charging slot.
- **Stack** holds the undo history.
- **Linked lists** back the graph, the rack entries and the fleet.

## Project structure

```
src/        All source code (main.cpp plus one .h/.cpp pair per class)
data/       Text files loaded at startup
```

| Data file | Contents |
|-----------|----------|
| `locations.txt` | Location ID, type (RACK, PACKING or CHARGING), name |
| `pathways.txt` | Connections between locations, with distance and OPEN or BLOCKED status |
| `racks.txt` | Which rack sits at which location |
| `products.txt` | Product ID, name, unit weight, reorder level |
| `inventory.txt` | Product ID, rack ID, quantity |
| `orders.txt` | Order ID, customer ID, products and quantities |
| `robots.txt` | Robot ID, type, location, battery, capacity |
| `chargers.txt` | Charging station ID, location, slots |

Lines starting with `#` are comments. Invalid lines are reported and skipped, for example a robot whose capacity does not match its type.

## Build and run

You need a C++ compiler such as `g++`. Run these from the project root, because the program reads the `data/` folder by relative path.

```
g++ -std=c++11 -o warehouse src/*.cpp
./warehouse
```

On Windows, run `warehouse.exe` instead of `./warehouse`.

## Example start-up

```
=================================================
   INTELLIGENT WAREHOUSE ROBOT SYSTEM
=================================================
Loaded 15 locations from data/locations.txt.
Loaded 22 pathways from data/pathways.txt.
Loaded 11 racks from data/racks.txt.
Loaded 10 products from data/products.txt.
Loaded 17 inventory records from data/inventory.txt.
Loaded 11 orders from data/orders.txt.
Loaded 6 robots from data/robots.txt.
Loaded 2 charging stations from data/chargers.txt.

================== MAIN MENU ==================
  1. Warehouse Layout & Pathways
  2. Product Inventory & Racks
  3. Customer Orders
  4. Robot Fleet
  5. Robot Assignment & Picking Plans
  6. Order Picking & Delivery Execution
  7. Battery & Charging
  8. Operation History & Undo
  0. Exit
```

## Try this

1. Open **1**, then **9** to find the shortest route between two locations.
2. Open **2**, then **2** to see the AVL tree structure, then insert a product and look again.
3. Block a pathway in **1**, update a quantity in **2**, then open **8** and undo both.

## About

A university project. Author: Ali Hassan.
