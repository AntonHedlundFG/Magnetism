# Magnetism - Physics Assignment
This prototype was implemented for a Physics course at FutureGames. It simulates simple mono-polar magnet behaviour, and uses custom physics for magnetic force, movement, and collisions, as well as custom ray-to-sphere intersection to allow the user to select the spherical magnets in runtime.

Considering monopolar magnets do not exist (?), a more appropriate name for the prototype might be "Interactions between electrically charged particles". Magnets just sound cooler.

![Visual Example](/Pics/Visual1.png)

## Implementation
All behaviour is simulated in [UMagnetismPhysicsSystem](/Source/Magnetism/Private/MagnetismPhysicsSystem.cpp). It is a UGameInstanceSubsystem, as well as a FTickableGameObject. Each tick it performs the following tasks in order:
- Apply all magnetic forces between each pair of magnets, using an adaptation of Coulomb's Law.
- Update the location of all magnets based on their velocities, and applies drag.
- Check all magnet pairs for Magnet-To-Magnet collisions.
- Check each magnet for collisions with the bounding box.

## Multi-Threaded solution
Initially, the prototype was implemented with a single-threaded solution. With it, I could run the executable with approximately 800 magnets before dipping below 60FPS.

For both magnetic forces and magnet-to-magnet collisions, the physics system needs to use nested for loops to iterate over each **pair** of magnets. For these nested loops, I replaced the outer loop with ParallelFor function calls. 
ParallelFor is a function in Unreal Engine that functions as a for loop, but automatically distributes work among threads. It cannot guarantee the **order** in which the loop is handled, but that is fine for this use.

By updating to a multi-threaded model, I could easily reach a count of 2000 magnets before closing in on 60FPS.
