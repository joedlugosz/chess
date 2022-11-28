## Searching

[search.c](../src/search.c)

The searching algorithm has the following features:

 - Negamax search
 - Alpha-beta pruning
 - Quiescence search
 - Move ordering heuristic
 - Killer move heuristic
 - Transposition table with best move

This is a recursive depth-first search to a fixed initial depth. At the root of
the search, the `depth` argument to `search_position` is set to the intended
depth, and it is decreased with each recursion down to zero at full depth. To
avoid the horizon effect, the search is extended beyond the initial depth in a
quiescence search considering only moves which are interesting enough, for
example captures. Beyond the horizon, the depth is negative. There is a mutual
recursion between `search_position` and `search_move`. `search_position`
generates multiple moves for a position, calling `search_move` for each one.
`search_move` makes the move and calls into `search_position` with the new
position. 

For efficiency, `search_position` works in two phases. The first phase involves
using heuristics or hashed information to try to return early by finding a beta
cutoff without the hard work of an exhaustive search. Even if this doesn't work,
the process usually updates alpha to a good starting value. The second phase
occurs if an early exit was not achieved. This involves a search through all
remaining possible moves for this position, with an earlier exit if a beta
cutoff is found.

A list of pseudo-legal moves is generated. These are moves which appear to be
legal without considering check. It is easier to test for check once the move
has been made, in then return early from search_move if it turns out to be
illegal. In front of the search horizon, a list is generated for a normal
search, with all moves that can be made for all pieces. At and beyond the
horizon, a separate function is used to generate a list for a quiescence search.
Moves are sorted by the generation functions according to a move ordering
heuristic. To assist with sorting, moves are are stored as a linked list within
a buffer. Pseudo-legal moves are searched, updating a count of genuine legal
moves. If no genuine legal moves were found, it means a checkmate or stalemate.

Checkmate and stalemate scoring are handled by `search_position` rather than
[`evaluate`](../src/evaluate.c). Checkmate produces an extreme score which is
reduced by the distance from the player's game position to mate, so that the
shortest path to mate has the strongest score. Stalemate causes a draw score of
zero, which is a goal for the losing side.

Transposition table
  higher levels only
