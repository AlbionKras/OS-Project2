CS 4348 Project 2
Albion Krasniqi
November 10, 2024


November 10, 2024 — 10:35 pm

**Thoughts So Far**
I read the project requirements and the sample output. 
There are several synchronization constraints:
3 tellers / 50 customers
Only 2 tellers in the safe
Only 1 teller can talk to manager
Only 2 customers can enter the bank at once
Need individual teller ↔ customer communication
I will use C, std::thread, and POSIX semaphores so it compiles on cs1/cs2.

**Plan for This Session**
Make a new local git repo
Create devlog.md
Create project .c file
Define shared resources & thread function stubs

**Actions Taken**
Created repo and added devlog.md
Wrote global semaphore declarations, queue, mutexes, atomic counters
Stubbed teller() and customer() functions

**Git Commits This Session**
git add devlog.md
git commit -m "Initial commit: Created devlog.md and repository"
git add main.c
git commit -m "Add shared resources and threading structure"

 November 10, 2024 — 10:50 pm
 
**Reflection**
Goal achieved. The program has been set up and has proper structure.
Next: Implement main() thread creation 


November 10, 2024 — 11:11 pm

**Thoughts So Far**
Now that structure is working, I will implement the runtime piece-by-piece:
Start tellers → open bank → launch customers

**Plan for This Session**
Implement main() thread startup
Initialize all semaphores

**Actions Taken**
Added initialization of all semaphores
Added thread creation for tellers and customers
Added bank open message
Added shutdown process

**Git Commits This Session**
git add bank.c
git commit -m "Implement main(): initialize semaphores, start teller and customer threads"
git commit -m "Add shutdown logic to close bank when all customers served"

**Reflection 11:24 pm**
Everything launches successfully and shuts down cleanly and now I will implement detailed teller/customer behavior.
