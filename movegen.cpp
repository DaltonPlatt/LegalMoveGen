/*
  LegalMoveGen, an incomplete UCI chess engine prototype derived from Stockfish 2.2.2 and Kaissa 1.00.
  Copyright (C) 2012 Dr. Dmitri A. Gusev

  Stockfish 2.2.2 is a free, open-source UCI chess engine derived from Glaurung 2.1.
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2012 Marco Costalba, Joona Kiiski, Tord Romstad

  Kaissa 1.00 is a free, open-source chess program derived from the original Kaissa.
  Copyright (C) 1972-1980 G.M. Adelson-Velsky, V.L. Arlazarov, and M.V. Donskoy (Kaissa authors);
  Other contributors: A.V. Uskov, A.R. Bitman, A. Barayev, A. Leman, and M. Rosenfeld
  Copyright (C) 1990-1991 JV ParaGraph, intellectual property rights transferred to DISCo in 1994;
  Authors of Kaissa 1.00: M.V. Donskoy, A.V. Dubetz, M.Yu. Karaev, V.A. Kokin,
  D.V. Posvjansky, I.R. Shabalin, A.G. Sidorovitch, E.A. Sokolinsky.
  Sources used by written permission from DISCo.

  LegalMoveGen is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  LegalMoveGen is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <algorithm>
#include <cassert>
#include <iostream>

#include "movegen.h"

/// Constructor creates an empty root move list
RootMoveList::RootMoveList()
{
   length = 0;
   currentPos = 0;
}

/// Is the root move list empty?
bool RootMoveList::IsEmpty() const
{
   return (length == 0);
}

/// Is the root move list full?
bool RootMoveList::IsFull() const
{
   return (length == MAX_MOVES);
}

/// Get the length of the root move list
int RootMoveList::GetLength() const
{
   return length;
}

/// Insert a root move into the root move list
void RootMoveList::Insert(RootMove item)
{
   rmlist[length] = item;
   length++;
}

/// Delete a root move from the root move list
void RootMoveList::Delete(RootMove item)
{
   int index = 0;
   while (index < length && item != rmlist[index].pv[0])
      index++;
   if (index < length) // item found
   {
      rmlist[index] = rmlist[length-1];
      length--;
   }
}

/// Is a certain move in the root move list?
bool RootMoveList::IsThere(RootMove item) const
{
   int index = 0; // Index variable
   while (index < length && item != rmlist[index].pv[0])
      index++;
   return (index < length);
}

/// Reset the current position in the root move list
void RootMoveList::ResetList()
{
   currentPos = 0;
}

/// Does the list have a next item?
bool RootMoveList::HasNext() const
{
   return (currentPos != length);
}

/// Get the next item from the root move list
RootMove RootMoveList::GetNextItem()
{
   return rmlist[currentPos++];
}

// In RootMoveList class definition
RootMove GetRandomMove();

// In RootMoveList class implementation
RootMove RootMoveList::GetRandomMove()
{
    // Check if the list is not empty
    if (IsEmpty())
    {
        cerr << "Error: Trying to get a random move from an empty move list\n";
        // Return a default move or handle the error as appropriate for your program
        return RootMove(); // You may need to define a default constructor for RootMove
    }

    // Generate a random index within the range of valid moves
    int randomIndex = rand() % GetLength();

    // Set the current position to the randomly chosen index
    currentPos = randomIndex;

    // Return the move at the randomly chosen index
    return rmlist[currentPos];
}

/// Clear the list of root moves
void RootMoveList::ClearList()
{
	length=0;
	ResetList();
}

/// Generate the list of all legal root moves from a given position
void RootMoveList::generate_from_position(Position& pos)
{
	Color sideToMove; // color of the side whose turn it is to make a move
	Color c;
	Square ksq;       // king square of the side to move
	Square dsq;       // current square of the line of defense
	Square esq;       // potential en passant square
	Piece p, p_our, p_their;
	PieceType pt;
	Square s, sq;
	const Square* ptrPieceList;
	Move m;
	int dir, opt;
	bool lookForCheckDefense = true; // Set to false if in check that cannot be defended against, i.e.
	                                 // either multiple check, or check from an adjacent square
	bool lookForCheckerCapture;      // Set to false if in multiple check, true otherwise:
	                                 // Cannot respond to a multiple check by capturing a checker
	int numCheckers;                 // Store the number of checkers
	Bitboard checkers;               // Store the bitboard of checkers
	Bitboard attackers;              // Store the bitboard of attackers (of a lonely checker)
	Bitboard defenders;              // Store the bitboard of defenders (of the king)

	// Whose turn is it? (We call that side "side to move", "our side", "us".)
	sideToMove = pos.side_to_move();

	// Is our king in check? Are any of our pieces pinned?
	CheckInfo ci(pos, sideToMove, lookForCheckDefense);
	checkers = pos.checkers();
	// Where is the king of the side to move ("our side", "us")?
	ksq = pos.king_square(sideToMove);

	if (pos.in_check()) // if in check
	{
		// How many checkers are there?
		numCheckers = pos.how_many_checkers();
		// If numCheckers>1, then capture of a checker is not an option:
		lookForCheckerCapture = (numCheckers == 1);

		if (lookForCheckerCapture)
		{   /// find all available moves that allow us to capture the checker
			/// and add these moves to the list of legal root moves
			Square csq = first_1(checkers); // checker square
			print_square(csq); // debugging
			attackers = pos.attackers_to(csq, ksq, sideToMove);
			print_bitboard(attackers, "Attackers:"); // debugging
			m.to = csq;
			while(attackers.any())
			{
				s = pop_1st_bit(attackers);
				if(!ci.pinned[s]) // a pinned attacker cannot capture a checker
				{
					m.from = s;
					m.moveFlags.reset();      // initialize all moveFlags to zeros to begin with
					m.moveFlags[M_CAPTURE]=1; // a capture of a checker... is a capture
					m.score = 0;              // not computing score correctly yet
					p = pos.piece_on(s);
					if (sideToMove) // black's turn to move
					{
						if (p==BLACK_PAWN)
						{
							if (pos.pawn_is_passed(sideToMove,csq))
								m.moveFlags[M_PASSAGE]=1;
							if (ranksOfSquares[s]==RANK_2)
							{   // checker capture by black pawn with promotion
								m.promotionPieceType = KNIGHT;
								m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
								Insert(RootMove(m));
								m.promotionPieceType = BISHOP;
								m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
								Insert(RootMove(m));
								m.promotionPieceType = ROOK;
								m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
								Insert(RootMove(m));
								m.promotionPieceType = QUEEN;
								m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
								Insert(RootMove(m));
							}
							else
							{   // checker capture by black pawn w/o promotion
								m.promotionPieceType = NO_PIECE_TYPE;
								m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
								Insert(RootMove(m));
							}
						}
						else
						{   // checker capture by black non-pawn
							m.promotionPieceType = NO_PIECE_TYPE;
							m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
							Insert(RootMove(m));
						}
					}
					else // white's turn to move
					{
						if (p==WHITE_PAWN)
						{
							if (pos.pawn_is_passed(sideToMove,csq))
								m.moveFlags[M_PASSAGE]=1;
							if(ranksOfSquares[s]==RANK_7)
							{   // checker capture by white pawn with promotion
								m.promotionPieceType = KNIGHT;
								m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
								Insert(RootMove(m));
								m.promotionPieceType = BISHOP;
								m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
								Insert(RootMove(m));
								m.promotionPieceType = ROOK;
								m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
								Insert(RootMove(m));
								m.promotionPieceType = QUEEN;
								m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
								Insert(RootMove(m));
							}
							else
							{   // checker capture by white pawn w/o promotion
								m.promotionPieceType = NO_PIECE_TYPE;
								m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
								Insert(RootMove(m));
							}
						}
						else
						{   // checker capture by white non-pawn
							m.promotionPieceType = NO_PIECE_TYPE;
							m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
							Insert(RootMove(m));
						}
					}
				}
			}
			// Now let's find out if the checker could be captured en passant:
			if (sideToMove) // black's turn to move
			{
				if ((pos.piece_on(csq)==WHITE_PAWN)&&(ranksOfSquares[csq]==RANK_4))
				{
					esq = Square(csq-8);
					if (pos.piece_on(esq)==EN_PASSANT)
					{   // Is there a black pawn capable of capturing
						// the checker pawn en passant?
						dir=1;
						while (protoMoves[WHITE_PAWN][esq][dir][0]!=NO) // with pawns, we won't exhaust directions
						{
							opt=0;
							do
							{
								s = protoMoves[WHITE_PAWN][esq][dir][opt];
								if((pos.piece_on(s)==BLACK_PAWN)&&(!ci.pinned[s]))
								{   // found an eligible black pawn
									m.to = esq;
									m.from = s;
									m.moveFlags.reset(); // initialize all moveFlags to zeros to begin with
									m.score = 0;         // not computing score correctly yet
									m.promotionPieceType = NO_PIECE_TYPE; // en passant capture cannot be a promotion!
									m.moveFlags[M_CAPTURE]=1; // a capture of a checker... is a capture
									m.moveFlags[M_EN_PASSANT]=1;
									if (pos.pawn_is_passed(sideToMove,esq))
										m.moveFlags[M_PASSAGE]=1;
									m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
									Insert(RootMove(m));
								}
								opt++;
							}
							while (protoMoves[WHITE_PAWN][esq][dir][opt]!=NO);
							dir++;
						}
					}
				}
			}
			else // white's turn to move
			{
				if ((pos.piece_on(csq)==BLACK_PAWN)&&(ranksOfSquares[csq]==RANK_5))
				{
					esq = Square(csq+8);
					if (pos.piece_on(esq)==EN_PASSANT)
					{
						// Is there a white pawn capable of capturing
						// the checker pawn en passant?
						dir=1;
						while (protoMoves[BLACK_PAWN][esq][dir][0]!=NO) // with pawns, we won't exhaust directions
						{
							opt=0;
							do
							{
								s = protoMoves[BLACK_PAWN][esq][dir][opt];
								if((pos.piece_on(s)==WHITE_PAWN)&&(!ci.pinned[s]))
								{   // found an eligible white pawn
									m.to = esq;
									m.from = s;
									m.moveFlags.reset(); // initialize all moveFlags to zeros to begin with
									m.score = 0;         // not computing score correctly yet
									m.promotionPieceType = NO_PIECE_TYPE; // en passant capture cannot be a promotion!
									m.moveFlags[M_CAPTURE]=1; // a capture of a checker... is a capture
									m.moveFlags[M_EN_PASSANT]=1;
									if (pos.pawn_is_passed(sideToMove,esq))
										m.moveFlags[M_PASSAGE]=1;
									m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
									Insert(RootMove(m));
								}
								opt++;
							}
							while (protoMoves[WHITE_PAWN][esq][dir][opt]!=NO);
							dir++;
						}
					}
				}
			}
		}
		if (lookForCheckDefense)
		{   /// find all available check defense moves and
			/// add them to the list of legal root moves
			opt = 0;
			dsq = ci.lineOfDefense[opt];
			do
			{
				defenders = pos.defenders_of(dsq, sideToMove);
				m.to = dsq;
				while(defenders.any())
				{
					s = pop_1st_bit(defenders);
					if(!ci.pinned[s]) // a pinned defender cannot defend against a check
					{
						m.from = s;
						m.moveFlags.reset(); // initialize all moveFlags to zeros to begin with
						m.score = 0;         // not computing score correctly yet
						m.moveFlags[M_CHECK_DEFENSE]=1;
						p = pos.piece_on(s);
						if (sideToMove) // black's turn to move
						{
							if (p==BLACK_PAWN)
							{
								if (pos.pawn_is_passed(sideToMove,dsq))
									m.moveFlags[M_PASSAGE]=1;
								if (ranksOfSquares[s]==RANK_2)
								{   // check defense by black pawn with promotion
									m.promotionPieceType = KNIGHT;
									m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
									if(!is_check(m))
										m.moveFlags[M_QUIET] = 1;
									Insert(RootMove(m));
									m.promotionPieceType = BISHOP;
									m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
									if(!is_check(m))
										m.moveFlags[M_QUIET] = 1;
									Insert(RootMove(m));
									m.promotionPieceType = ROOK;
									m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
									if(!is_check(m))
										m.moveFlags[M_QUIET] = 1;
									Insert(RootMove(m));
									m.promotionPieceType = QUEEN;
									m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
									if(!is_check(m))
										m.moveFlags[M_QUIET] = 1;
									Insert(RootMove(m));
								}
								else
								{   // check defense by black pawn w/o promotion
									m.promotionPieceType = NO_PIECE_TYPE;
									m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
									if(!is_check(m))
										m.moveFlags[M_QUIET] = 1;
									Insert(RootMove(m));
								}
							}
							else
							{   // check defense by black non-pawn
								m.promotionPieceType = NO_PIECE_TYPE;
								m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
								if(!is_check(m))
									m.moveFlags[M_QUIET] = 1;
								Insert(RootMove(m));
							}
						}
						else // white's turn to move
						{
							if (p==WHITE_PAWN)
							{
								if (pos.pawn_is_passed(sideToMove,dsq))
									m.moveFlags[M_PASSAGE]=1;
								if (ranksOfSquares[s]==RANK_7)
								{
									// check defense by white pawn with promotion
									m.promotionPieceType = KNIGHT;
									m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
									if(!is_check(m))
										m.moveFlags[M_QUIET] = 1;
									Insert(RootMove(m));
									m.promotionPieceType = BISHOP;
									m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
									if(!is_check(m))
										m.moveFlags[M_QUIET] = 1;
									Insert(RootMove(m));
									m.promotionPieceType = ROOK;
									m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
									if(!is_check(m))
										m.moveFlags[M_QUIET] = 1;
									Insert(RootMove(m));
									m.promotionPieceType = QUEEN;
									m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
									if(!is_check(m))
										m.moveFlags[M_QUIET] = 1;
									Insert(RootMove(m));
								}
								else
								{   // check defense by white pawn w/o promotion
									m.promotionPieceType = NO_PIECE_TYPE;
									m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
									if(!is_check(m))
										m.moveFlags[M_QUIET] = 1;
									Insert(RootMove(m));
								}
							}
							else
							{   // check defense by white non-pawn
								m.promotionPieceType = NO_PIECE_TYPE;
								m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
								if(!is_check(m))
									m.moveFlags[M_QUIET] = 1;
								Insert(RootMove(m));
							}
						}
					}
				}
				opt++;
			}
			while ((dsq=ci.lineOfDefense[opt])!=NO);
			//print_bitboard(defenders, "Defenders:"); // debugging
		}
		/// find all available check evasions and
		/// add them to the list of legal root moves
		p_our = Piece(WHITE_KING + sideToMove);
		dir=0;
		m.from = ksq;
		while ((dir<MAX_DIRECTIONS)&&((s=protoMoves[p_our][ksq][dir][0])!=NO))
		{
			// castling is not allowed when in check
			p = pos.piece_on(s);
			if(p > EN_PASSANT) // Square s is not empty
			{
				c = colorOf(p);
				if (c==(~sideToMove))
				{   /// Found an enemy piece, our king can try to capture it.
					// If the piece is a lonely checker, then our king has tried already!
					if((checkers&s).none()||(!lookForCheckerCapture))
					{
						if (pos.not_defended(s,~sideToMove))
						{   // capture an undefended non-single enemy checker
							// or an undefended enemy non-checker within reach:
							m.to = s;
							m.moveFlags.reset(); // initialize all moveFlags to zeros to begin with
							m.score = 0;         // not computing score correctly yet
							m.moveFlags[M_EVASION]=1;
							m.moveFlags[M_CAPTURE]=1;
							m.promotionPieceType = NO_PIECE_TYPE;
							m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
							Insert(RootMove(m));
						}
					}
				}
			}
			else // Square s is empty
			{   // Our king can move to Square s if the square is not "defended":
				if (pos.not_defended(s,~sideToMove))
				{
					m.to = s;
					m.moveFlags.reset(); // initialize all moveFlags to zeros to begin with
					m.score = 0;         // not computing score correctly yet
					m.moveFlags[M_EVASION]=1;
					m.promotionPieceType = NO_PIECE_TYPE;
					m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
					if(!is_check(m))
						m.moveFlags[M_QUIET] = 1;
					Insert(RootMove(m));
				}
			}
			dir++;
		}
		/// If the list of legal root moves is empty,
		/// then we're checkmated in the starting position:
		/// Return MOVE_NONE
		/// Otherwise test for a draw
		Move move_none;
		move_none.from=A1;
		move_none.to=A1;
		move_none.promotionPieceType=NO_PIECE_TYPE;
		move_none.moveFlags.reset();
		move_none.score=0;
		if (length)
		{   // Can move and check for draw:
			if (pos.is_draw<true>())
			{
				move_none.moveFlags[M_MOVE_NONE]=1;
				move_none.moveFlags[M_DRAW]=1;
				ClearList();
				Insert(RootMove(move_none));
			}
		}
		else // In check and no moves: Checkmate
		{
			move_none.moveFlags[M_MOVE_NONE]=1;
			move_none.moveFlags[M_CHECKMATE]=1;
			Insert(RootMove(move_none));
		}
	}
	else // if not in check
	{
		// test for a draw
		Move move_none;
		move_none.from=A1;
		move_none.to=A1;
		move_none.promotionPieceType=NO_PIECE_TYPE;
		move_none.moveFlags.reset();
		move_none.score=0;
		if (pos.is_draw<true>())
		{
			move_none.moveFlags[M_MOVE_NONE]=1;
			move_none.moveFlags[M_DRAW]=1;
			ClearList();
			Insert(RootMove(move_none));
		}
		else
		{
			// test for castling options
			if (sideToMove) // black's turn to move
			{
				if(pos.can_castle(BLACK_OO))
				{
					if((pos.square_is_empty(F8))&&(pos.square_is_empty(G8))&&(pos.not_defended(F8,~sideToMove))&&
						(pos.not_defended(G8,~sideToMove)))
					{
						m.from = E8;
						m.to = G8;
						m.moveFlags.reset(); // initialize all moveFlags to zeros to begin with
						m.score = 0;         // not computing score correctly yet
						m.promotionPieceType = NO_PIECE_TYPE;
						m.moveFlags[M_CASTLE_SHORT]=1;
						m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
						if(!is_check(m))
							m.moveFlags[M_QUIET] = 1;
						Insert(RootMove(m));
					}
				}
				if(pos.can_castle(BLACK_OOO))
				{
					if((pos.square_is_empty(D8))&&(pos.square_is_empty(C8))&&(pos.square_is_empty(B8))&&
						(pos.not_defended(D8,~sideToMove))&&(pos.not_defended(C8,~sideToMove)))
					{
						m.from = E8;
						m.to = C8;
						m.moveFlags.reset(); // initialize all moveFlags to zeros to begin with
						m.score = 0;         // not computing score correctly yet
						m.promotionPieceType = NO_PIECE_TYPE;
						m.moveFlags[M_CASTLE_LONG]=1;
						m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
						if(!is_check(m))
							m.moveFlags[M_QUIET] = 1;
						Insert(RootMove(m));
					}
				}
			}
			else // white's turn to move
			{
				if(pos.can_castle(WHITE_OO))
				{
					if((pos.square_is_empty(F1))&&(pos.square_is_empty(G1))&&(pos.not_defended(F1,~sideToMove))&&
						(pos.not_defended(G1,~sideToMove)))
					{
						m.from = E1;
						m.to = G1;
						m.moveFlags.reset(); // initialize all moveFlags to zeros to begin with
						m.score = 0;         // not computing score correctly yet
						m.promotionPieceType = NO_PIECE_TYPE;
						m.moveFlags[M_CASTLE_SHORT]=1;
						m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
						if(!is_check(m))
							m.moveFlags[M_QUIET] = 1;
						Insert(RootMove(m));
					}
				}
				if(pos.can_castle(WHITE_OOO))
				{
					if((pos.square_is_empty(D1))&&(pos.square_is_empty(C1))&&(pos.square_is_empty(B1))
						&&(pos.not_defended(D1,~sideToMove))&&(pos.not_defended(C1,~sideToMove)))
					{
						m.from = E1;
						m.to = C1;
						m.moveFlags.reset(); // initialize all moveFlags to zeros to begin with
						m.score = 0;         // not computing score correctly yet
						m.promotionPieceType = NO_PIECE_TYPE;
						m.moveFlags[M_CASTLE_LONG]=1;
						m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
						if(!is_check(m))
							m.moveFlags[M_QUIET] = 1;
						Insert(RootMove(m));
					}
				}
			}

			/// generate the other moves (captures and quiet ones);
			/// don't forget about the en passant captures;
			/// make sure that the pinned pieces are only allowed
			/// to move along the direction of their pinning!

			// generate pawn moves
			pt = PAWN;
			p_our = make_piece(sideToMove, pt);
			int pcnt = pos.piece_count(sideToMove, pt);
			ptrPieceList = pos.piece_list(sideToMove, pt);
			for (int i=0; i<pcnt; i++)
			{
				sq = ptrPieceList[i];
				dir=0;
				while (protoMoves[p_our][sq][dir][0]!=NO) // with pawns, we won't exhaust directions
				{
					opt=0;
					do
					{
						s = protoMoves[p_our][sq][dir][opt];
						p = pos.piece_on(s);
						c = colorOf(p);

						if(c==(~sideToMove)||(p==EN_PASSANT))
						{   // encountered an enemy piece or an en passant square

							if (dir)
							{   // can capture unless pinned along another direction
								if ((ci.pinned&sq).none()||same_line(sq,s,ksq))
								{
									m.from = sq;
									m.to = s;
									m.moveFlags.reset(); // initialize all moveFlags to zeros to begin with
									m.score = 0;         // not computing score correctly yet
									m.moveFlags[M_CAPTURE]=1;
									if (pos.pawn_is_passed(sideToMove,s))
										m.moveFlags[M_PASSAGE]=1;

									if (sideToMove) // black's turn to move
									{
										if (ranksOfSquares[sq]==RANK_2)
										{   // capture by black pawn with promotion
											m.promotionPieceType = KNIGHT;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											Insert(RootMove(m));
											m.promotionPieceType = BISHOP;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											Insert(RootMove(m));
											m.promotionPieceType = ROOK;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											Insert(RootMove(m));
											m.promotionPieceType = QUEEN;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											Insert(RootMove(m));
										}
										else if(p==EN_PASSANT)
										{   // en passant capture by black pawn
											m.moveFlags[M_EN_PASSANT]=1;
											if(ranksOfSquares[ksq]==RANK_4)
											{
												bool enemy_rook_or_queen_found = false;
												if ((Rank4BB&(pos.pieces(ROOK,QUEEN,WHITE))).any())
												{   // cannot capture en passant if an enemy rook or queen opens up
													if (filesOfSquares[ksq] < filesOfSquares[sq])
													{
														for(Square i = Square(ksq+1);filesOfSquares[i]<=FILE_H; i = Square(i+1))
														{
															p_their = pos.piece_on(i);
															if((p_their==WHITE_ROOK)||(p_their==WHITE_QUEEN))
															{
																enemy_rook_or_queen_found = true;
																break;
															}
															else if (((colorOf(p_their)==BLACK)&&(i!=sq))||
																((colorOf(p_their)==WHITE)&&(filesOfSquares[i]!=filesOfSquares[s])))
															break;
														}
													}
													else // filesOfSquares[ksq] > filesOfSquares[sq]
													{
														for(Square i = Square(ksq-1);filesOfSquares[i]>=FILE_A; i = Square(i-1))
														{
															p_their = pos.piece_on(i);
															if((p_their==WHITE_ROOK)||(p_their==WHITE_QUEEN))
															{
																enemy_rook_or_queen_found = true;
																break;
															}
															else if (((colorOf(p_their)==BLACK)&&(i!=sq))||
																((colorOf(p_their)==WHITE)&&(filesOfSquares[i]!=filesOfSquares[s])))
															break;
														}
													}
													if (enemy_rook_or_queen_found)
														break;
												}
											}
											m.promotionPieceType = NO_PIECE_TYPE;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											Insert(RootMove(m));
										}
										else
										{   // capture by black pawn without promotion
											m.promotionPieceType = NO_PIECE_TYPE;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											Insert(RootMove(m));
										}
									}
									else // white's turn to move
									{
										if (ranksOfSquares[sq]==RANK_7)
										{   // capture by white pawn with promotion
											m.promotionPieceType = KNIGHT;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											Insert(RootMove(m));
											m.promotionPieceType = BISHOP;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											Insert(RootMove(m));
											m.promotionPieceType = ROOK;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											Insert(RootMove(m));
											m.promotionPieceType = QUEEN;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											Insert(RootMove(m));
										}
										else if(p==EN_PASSANT)
										{   // en passant capture by white pawn
											m.moveFlags[M_EN_PASSANT]=1;
											if(ranksOfSquares[ksq]==RANK_5)
											{
												bool enemy_rook_or_queen_found = false;
												if ((Rank5BB&(pos.pieces(ROOK,QUEEN,BLACK))).any())
												{   // cannot capture en passant if an enemy rook or queen opens up
													if (filesOfSquares[ksq] < filesOfSquares[sq])
													{
														for(Square i = Square(ksq+1);filesOfSquares[i]<=FILE_H; i = Square(i+1))
														{
															p_their = pos.piece_on(i);
															if((p_their==BLACK_ROOK)||(p_their==BLACK_QUEEN))
															{
																enemy_rook_or_queen_found = true;
																break;
															}
															else if (((colorOf(p_their)==WHITE)&&(i!=sq))||
																((colorOf(p_their)==BLACK)&&(filesOfSquares[i]!=filesOfSquares[s])))
															break;
														}
													}
													else // filesOfSquares[ksq] > filesOfSquares[sq]
													{
														for(Square i = Square(ksq-1);filesOfSquares[i]>=FILE_A; i = Square(i-1))
														{
															p_their = pos.piece_on(i);
															if((p_their==BLACK_ROOK)||(p_their==BLACK_QUEEN))
															{
																enemy_rook_or_queen_found = true;
																break;
															}
															else if (((colorOf(p_their)==WHITE)&&(i!=sq))||
																((colorOf(p_their)==BLACK)&&(filesOfSquares[i]!=filesOfSquares[s])))
															break;
														}
													}
													if (enemy_rook_or_queen_found)
														break;
												}
											}
											m.promotionPieceType = NO_PIECE_TYPE;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											Insert(RootMove(m));
										}
										else
										{   // capture by white pawn without promotion
											m.promotionPieceType = NO_PIECE_TYPE;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											Insert(RootMove(m));
										}
									}
								}
							}
							else break; // pawn blocked by an enemy piece
						}
						else if (c == NO_COLOR)
						{   // can move forward (even if pinned along the vertical direction)
							if(!dir)
							{
								// cannot move forward a pawn pinned along a diagonal:
								if (((ci.pinned)&sq).any() && (file_distance(sq,ksq)));
								else
								{
									m.from = sq;
									m.to = s;
									m.moveFlags.reset(); // initialize all moveFlags to zeros to begin with
									if (pos.pawn_is_passed(sideToMove,s))
										m.moveFlags[M_PASSAGE]=1;
									m.score = 0;         // not computing score correctly yet
									if (sideToMove) // black's turn to move
									{
										if (ranksOfSquares[sq]==RANK_2)
										{   // black pawn promotion
											m.promotionPieceType = KNIGHT;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											if(!is_check(m))
												m.moveFlags[M_QUIET] = 1;
											Insert(RootMove(m));
											m.promotionPieceType = BISHOP;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											if(!is_check(m))
												m.moveFlags[M_QUIET] = 1;
											Insert(RootMove(m));
											m.promotionPieceType = ROOK;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											if(!is_check(m))
												m.moveFlags[M_QUIET] = 1;
											Insert(RootMove(m));
											m.promotionPieceType = QUEEN;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											if(!is_check(m))
												m.moveFlags[M_QUIET] = 1;
											Insert(RootMove(m));
										}
										else
										{   // black pawn moves forward
											m.promotionPieceType = NO_PIECE_TYPE;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											if(!is_check(m))
												m.moveFlags[M_QUIET] = 1;
											Insert(RootMove(m));
										}
									}
									else // white's turn to move
									{
										if (ranksOfSquares[sq]==RANK_7)
										{   // white pawn promotion
											m.promotionPieceType = KNIGHT;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											if(!is_check(m))
												m.moveFlags[M_QUIET] = 1;
											Insert(RootMove(m));
											m.promotionPieceType = BISHOP;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											if(!is_check(m))
												m.moveFlags[M_QUIET] = 1;
											Insert(RootMove(m));
											m.promotionPieceType = ROOK;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											if(!is_check(m))
												m.moveFlags[M_QUIET] = 1;
											Insert(RootMove(m));
											m.promotionPieceType = QUEEN;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											if(!is_check(m))
												m.moveFlags[M_QUIET] = 1;
											Insert(RootMove(m));
										}
										else
										{   // white pawn moves forward
											m.promotionPieceType = NO_PIECE_TYPE;
											m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
											if(!is_check(m))
												m.moveFlags[M_QUIET] = 1;
											Insert(RootMove(m));
										}
									}
								}
							}
						}
						else break; // pawn blocked by our own piece
						opt++;
					}
					while (protoMoves[p_our][sq][dir][opt]!=NO);
					dir++;
				}
			}

			// generate KNIGHT moves
			pt = KNIGHT;
			p_our = make_piece(sideToMove, pt);
			pcnt = pos.piece_count(sideToMove, pt);
			ptrPieceList = pos.piece_list(sideToMove, pt);
			for (int i=0; i<pcnt; i++)
			{
				sq = ptrPieceList[i];
				dir=0;
				while ((dir<MAX_DIRECTIONS)&&((s=protoMoves[p_our][sq][dir][0])!=NO))
				{
					p = pos.piece_on(s);
					c = colorOf(p);
					if(c==(~sideToMove))
					{
						if (((ci.pinned)&sq).none())
						{   // the knight is not pinned, so it can capture
							m.from = sq;
							m.to = s;
							m.moveFlags.reset(); // initialize all moveFlags to zeros to begin with
							m.score = 0;         // not computing score correctly yet
							m.moveFlags[M_CAPTURE]=1;
							m.promotionPieceType = NO_PIECE_TYPE;
							m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
							Insert(RootMove(m));
						};
					}
					else if (c==NO_COLOR)
					{
						if (((ci.pinned)&sq).none())
						{   // the knight is not pinned, so it can move
							m.from = sq;
							m.to = s;
							m.moveFlags.reset(); // initialize all moveFlags to zeros to begin with
							m.score = 0;         // not computing score correctly yet
							m.promotionPieceType = NO_PIECE_TYPE;
							m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
							if(!is_check(m))
								m.moveFlags[M_QUIET] = 1;
							Insert(RootMove(m));
						};
					}

					dir++;
				}
			}

			// generate KING moves
			pt = KING;
			p_our = make_piece(sideToMove, pt);
			pcnt = pos.piece_count(sideToMove, pt);
			ptrPieceList = pos.piece_list(sideToMove, pt);
			for (int i=0; i<pcnt; i++)
			{
				sq = ptrPieceList[i];
				dir=0;
				while ((dir<MAX_DIRECTIONS)&&((s=protoMoves[p_our][sq][dir][0])!=NO))
				{
					// one option per direction considered
					p = pos.piece_on(s);
					c = colorOf(p);
					if(pos.not_defended(s,~sideToMove))
					{
						if(c==(~sideToMove))
						{   // king captures an undefended enemy piece
							m.from = sq;
							m.to = s;
							m.moveFlags.reset(); // initialize all moveFlags to zeros to begin with
							m.score = 0;         // not computing score correctly yet
							m.moveFlags[M_CAPTURE]=1;
							m.promotionPieceType = NO_PIECE_TYPE;
							m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
							Insert(RootMove(m));
						}
						else if (c==NO_COLOR)
						{   // king moves to an available square
							m.from = sq;
							m.to = s;
							m.moveFlags.reset(); // initialize all moveFlags to zeros to begin with
							m.score = 0;         // not computing score correctly yet
							m.promotionPieceType = NO_PIECE_TYPE;
							m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
							if(!is_check(m))
								m.moveFlags[M_QUIET] = 1;
							Insert(RootMove(m));
						};
					}
					dir++;
				}
			}

			// generate QUEEN moves
			pt = QUEEN;
			p_our = make_piece(sideToMove, pt);
			pcnt = pos.piece_count(sideToMove, pt);
			ptrPieceList = pos.piece_list(sideToMove, pt);
			for (int i=0; i<pcnt; i++)
			{
				sq = ptrPieceList[i];
				dir=0;
				while ((dir<MAX_DIRECTIONS)&&(protoMoves[p_our][sq][dir][0]!=NO))
				{
					opt=0;
					do
					{
						s = protoMoves[p_our][sq][dir][opt];
						p = pos.piece_on(s);
						c = colorOf(p);
						if(c==(~sideToMove))
						{
							if (((ci.pinned)&sq).none()||same_line(sq,s,ksq))
							{   // the queen can capture freely if not pinned or along the pinning direction
								m.from = sq;
								m.to = s;
								m.moveFlags.reset(); // initialize all moveFlags to zeros to begin with
								m.score = 0;         // not computing score correctly yet
								m.moveFlags[M_CAPTURE]=1;
								m.promotionPieceType = NO_PIECE_TYPE;
								m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
								Insert(RootMove(m));
								break;
							}
							else break;
						}
						else if (c==NO_COLOR)
						{
							if (((ci.pinned)&sq).none()||same_line(sq,s,ksq))
							{   // the queen can move freely if not pinned or along the pinning direction
								m.from = sq;
								m.to = s;
								m.moveFlags.reset(); // initialize all moveFlags to zeros to begin with
								m.score = 0;         // not computing score correctly yet
								m.promotionPieceType = NO_PIECE_TYPE;
								m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
								if(!is_check(m))
									m.moveFlags[M_QUIET] = 1;
								Insert(RootMove(m));
							}
						}
						else break; // blocked by our own piece
						opt++;
					}
					while (protoMoves[p_our][sq][dir][opt]!=NO);
					dir++;
				}
			}

			// generate ROOK moves
			pt = ROOK;
			p_our = make_piece(sideToMove, pt);
			pcnt = pos.piece_count(sideToMove, pt);
			ptrPieceList = pos.piece_list(sideToMove, pt);
			for (int i=0; i<pcnt; i++)
			{
				sq = ptrPieceList[i];
				dir=0;
				while (protoMoves[p_our][sq][dir][0]!=NO)
				{
					opt=0;
					do
					{
						s = protoMoves[p_our][sq][dir][opt];
						p = pos.piece_on(s);
						c = colorOf(p);
						if(c==(~sideToMove))
						{
							if (((ci.pinned)&sq).none()||same_line(sq,s,ksq))
							{   // the rook can capture freely if not pinned or along the pinning direction
								m.from = sq;
								m.to = s;
								m.moveFlags.reset(); // initialize all moveFlags to zeros to begin with
								m.score = 0;         // not computing score correctly yet
								m.moveFlags[M_CAPTURE]=1;
								m.promotionPieceType = NO_PIECE_TYPE;
								m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
								Insert(RootMove(m));
								break;
							}
							else break;
						}
						else if (c==NO_COLOR)
						{
							if (((ci.pinned)&sq).none()||same_line(sq,s,ksq))
							{   // the rook can move freely if not pinned or along the pinning direction
								m.from = sq;
								m.to = s;
								m.moveFlags.reset(); // initialize all moveFlags to zeros to begin with
								m.score = 0;         // not computing score correctly yet
								m.promotionPieceType = NO_PIECE_TYPE;
								m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
								if(!is_check(m))
									m.moveFlags[M_QUIET] = 1;
								Insert(RootMove(m));
							}
						}
						else break; // blocked by our own piece
						opt++;
					}
					while (protoMoves[p_our][sq][dir][opt]!=NO);
					dir++;
				}
			}

			// generate BISHOP moves
			pt = BISHOP;
			p_our = make_piece(sideToMove, pt);
			pcnt = pos.piece_count(sideToMove, pt);
			ptrPieceList = pos.piece_list(sideToMove, pt);
			for (int i=0; i<pcnt; i++)
			{
				sq = ptrPieceList[i];
				dir=0;
				while (protoMoves[p_our][sq][dir][0]!=NO)
				{
					opt=0;
					do
					{
						s = protoMoves[p_our][sq][dir][opt];
						p = pos.piece_on(s);
						c = colorOf(p);
						if(c==(~sideToMove))
						{
							if (((ci.pinned)&sq).none()||same_line(sq,s,ksq))
							{   // the bishop can capture freely if not pinned or along the pinning direction
								m.from = sq;
								m.to = s;
								m.moveFlags.reset(); // initialize all moveFlags to zeros to begin with
								m.score = 0;         // not computing score correctly yet
								m.moveFlags[M_CAPTURE]=1;
								m.promotionPieceType = NO_PIECE_TYPE;
								m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
								Insert(RootMove(m));
								break;
							}
							else break;
						}
						else if (c==NO_COLOR)
						{
							if (((ci.pinned)&sq).none()||same_line(sq,s,ksq))
							{   // the bishop can move freely if not pinned or along the pinning direction
								m.from = sq;
								m.to = s;
								m.moveFlags.reset(); // initialize all moveFlags to zeros to begin with
								m.score = 0;         // not computing score correctly yet
								m.promotionPieceType = NO_PIECE_TYPE;
								m.moveFlags[M_ISCHECK] = pos.move_gives_check(m, ci);
								if(!is_check(m))
									m.moveFlags[M_QUIET] = 1;
								Insert(RootMove(m));
							}
						}
						else break; // blocked by our own piece
						opt++;
					}
					while (protoMoves[p_our][sq][dir][opt]!=NO);
					dir++;
				}
			}
		}
		if (!length) // No moves and not in check: Stalemate
		{
			move_none.moveFlags[M_MOVE_NONE]=1;
			move_none.moveFlags[M_STALEMATE]=1;
			move_none.moveFlags[M_DRAW]=1;
			Insert(RootMove(move_none));
		}
	}
}

/// Print the list of all legal root moves
void RootMoveList::print(Position& pos)
{
	RootMove rm;
	Color sideToMove = pos.side_to_move();
	int i=1;
	ResetList();
	while(HasNext())
	{
		rm = GetNextItem();
		pos.print_move(rm.pv[0], i);
		pos.print_flags(rm.pv[0]);
		i++;
	}
}
