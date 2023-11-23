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
// position.cpp implements class Position defined in position.h
#include <iostream>
#include <sstream>
#include <cstring>
#include "position.h"
using namespace std;
/// Position::init() is a static member function which initializes at startup
/// the various arrays used to compute hash keys and the piece square tables.
/// The latter is a two-step operation: First, the white halves of the tables
/// are copied from PSQT[] tables. Second, the black halves of the tables are
/// initialized by flipping and changing the sign of the white scores.
// [D.A.G. disabled computation of the piece square tables.]

// Hash keys:
Key Position::zobrist[NUM_PIECES][NUM_SQUARES];
// zobEp below is commented out by D.A.G.: EN_PASSANT is now a piece
//Key Position::zobEp[NUM_SQUARES];
Key Position::zobCastle[NUM_CASTLE_RIGHTS];
Key Position::zobSideToMove;
Key Position::zobExclusion;

//Score Position::pieceSquareTable[16][64];

void Position::init() {

  // D.A.G., 03/08/2012: We won't use RKISS to generate hash keys.
  //RKISS rk;

  // Hash key generation from protoMoves:
  for (Piece p = NO_PIECE; p <= BLACK_BISHOP; p=Piece(p+1))
  {
	  for (Square s = A1; s <= H8; s=Square(s+1))
	  {
		  zobrist[p][s] = 0;
		  PieceType pt = pieceTypeOf(p);
		  // Bishop doesn't leave footprint on its own square
		  // in order to reduce chances of canceling out of
		  // two bishops in opposing corners of the board:
		  if ((p!=NO_PIECE)&&(pt!=BISHOP))
			  zobrist[p][s]^=SquareBB[s];
		  if (p==EN_PASSANT)
		  {   // elongate en passant's "footprint":
			  if (rank_of(s)==RANK_3)
			  {
				  zobrist[p][s]^=SquareBB[Square(s-8)];
				  zobrist[p][s]^=SquareBB[Square(s-16)];
			  }
			  if (rank_of(s)==RANK_6)
			  {
				  zobrist[p][s]^=SquareBB[Square(s+8)];
				  zobrist[p][s]^=SquareBB[Square(s+16)];
			  }
		  }
		  int dir=0;
		  while ((dir<MAX_DIRECTIONS)&&(protoMoves[p][s][dir][0]!=NO))
		  {
			  int opt=0;
			  do
			  {
				  zobrist[p][s]^=SquareBB[protoMoves[p][s][dir][opt]];
				  opt++;
			  }
			  while(protoMoves[p][s][dir][opt]!=NO);
			  dir++;
		  }
		  if (colorOf(p)==BLACK)
			  zobrist[p][s]=zobrist[p][s].flip();
	  }
  }
  // D.A.G., 02/27/2012: Commented out zobEp,
  // EN_PASSANT is also a piece now
  /*
  for (Square s = A1; s <= H8; s=Square(s+1))
      zobEp[s] = rk.rand<Key>();
  */
  string keyvals[NUM_CASTLE_RIGHTS]={
	  "1001000100000000100010010000000010010001000000001000100100000000",
	  "0000000000100010000000000000000000001111000011110000101100001111",
	  "0000000000000000000000000000000011110000111100001101000011110000",
	  "0000000001000100000000000000000011111111111111111011110111111111",
	  "0000111100001011001011110000111100000000010000100000000000000000",
	  "0000111101001101000011110000111100101111000011110000110100001111",
	  "0000110101001111000011110000111111110000111100001111000011010100",
	  "0100111100001101000011110000111111111111111111111111101110111111",
	  "1111000011010001111100001111000000000000000000001000000100000000",
	  "1111000010110010111100001111000000001111000011110010110100001111",
	  "1101010011110000111100001111000011110000111100001111000010110100",
	  "1011010011110000111100001111000011111111111111111111101111011111",
	  "1111111110111011111111111111111100000000001010000000000000000000",
	  "1101111111111101111111111111111110001111000011110000111100001011",
	  "1111111111111111111111111111011111100000111100001111100011110000",
	  "1111111101101110111111110111011011111111011011101111111101110110"};
  for (int i = 0; i < NUM_CASTLE_RIGHTS; i++)
      zobCastle[i] = Key(keyvals[i],0,64);
  //print_bitboard(zobCastle[0]);
  string keyvalSideToMove="1111111111111111111111111111111111111111111111111111111111111111";
  zobSideToMove = Key(keyvalSideToMove,0,64);
  string keyvalExclusion="0101010110101010010101011010101001010101101010100101010110101010";
  zobExclusion  = Key(keyvalExclusion,0,64);

  // D.A.G., 02/25/2012: Piece-Square Tables are a simple way to assign values to
  // specific pieces on specific locations. A symmetric table is created for
  // each piece of each color, and values are assigned to each piece-square pair.
  /// D.A.G., 02/26/2012: Commented out computation of Piece-Square Tables for now.
  /*
  for (Piece p = W_PAWN; p <= W_KING; p++)
  {
      Score ps = make_score(PieceValueMidgame[p], PieceValueEndgame[p]);

      for (Square s = SQ_A1; s <= SQ_H8; s++)
      {
          pieceSquareTable[p][s] = ps + PSQT[p][s];
          pieceSquareTable[p+8][~s] = -pieceSquareTable[p][s];
      }
  }
  */
}

/// Position c'tors. Here we always create a copy of the original position
/// or the FEN string, we want the newborn Position object to be independent
/// from any external data so we detach state pointer from the source one.

void Position::copy(const Position& pos, int th)
{
  memcpy(this, &pos, sizeof(Position));
  startState = *st;
  st = &startState;
  threadID = th;
  nodes = 0;
  // D.A.G., 03/05/2012: Commented out the assertion statement.
  // assert(pos_is_ok());
}

// This constructor makes an instance of Position from a fen string:
Position::Position(const string& fen, bool isChess960, int th)
{
  from_fen(fen, isChess960);
  threadID = th;
}


/// Position::from_fen() initializes the position object with the given FEN
/// string. This function is not very robust - make sure that input FENs are
/// correct (this is assumed to be the responsibility of the GUI).

void Position::from_fen(const string& fenStr, bool isChess960)
{
/*
   A FEN string defines a particular position using only the ASCII character set.

   A FEN string contains six fields separated by a space. The fields are:

   1) Piece placement (from white's perspective). Each rank is described, starting
      with rank 8 and ending with rank 1; within each rank, the contents of each
      square are described from file A through file H. Following the Standard
      Algebraic Notation (SAN), each piece is identified by a single letter taken
      from the standard English names. White pieces are designated using upper-case
      letters ("PNBRQK") while Black take lowercase ("pnbrqk"). Blank squares are
      noted using digits 1 through 8 (the number of blank squares), and "/"
      separates ranks.

   2) Active color. "w" means white moves next, "b" means black.

   3) Castling availability. If neither side can castle, this is "-". Otherwise,
      this has one or more letters: "K" (White can castle kingside), "Q" (White
      can castle queenside), "k" (Black can castle kingside), and/or "q" (Black
      can castle queenside).

   4) En passant target square (in algebraic notation). If there's no en passant
      target square, this is "-". If a pawn has just made a 2-square move, this
      is the position "behind" the pawn. This is recorded regardless of whether
      there is a pawn in position to make an en passant capture.

   5) Halfmove clock. This is the number of halfmoves since the last pawn advance
      or capture. This is used to determine if a draw can be claimed under the
      fifty-move rule.

   6) Fullmove number. The number of the full move. It starts at 1, and is
      incremented after Black's move.
*/

  char col, row, token;
  size_t p;
  Square sq = A8;
  std::istringstream fen(fenStr);

  clear();
  fen >> std::noskipws;

  // 1. Piece placement
  while ((fen >> token) && !isspace(token))
  {
      if (isdigit(token))
          sq = Square(sq+ Square(token - '0')); // Advance the given number of files

      else if (token == '/')
          sq = make_square(FILE_A, Rank(rank_of(sq) - Rank(2)));

      else if ((p = PieceToChar.find(token)) != string::npos)
      {
          put_piece(Piece(p), sq);
          sq=Square(sq+1);
      }
  }

  // 2. Active color
  fen >> token;
  sideToMove = (token == 'w' ? WHITE : BLACK);
  fen >> token;

  // 3. Castling availability. Compatible with 3 standards: Normal FEN standard,
  // Shredder-FEN that uses the letters of the columns on which the rooks began
  // the game instead of KQkq and also X-FEN standard that, in case of Chess960,
  // if an inner rook is associated with the castling right, the castling tag is
  // replaced by the file letter of the involved rook, as for the Shredder-FEN.
  while ((fen >> token) && !isspace(token))
  {
      Square rsq;
      Color c = islower(token) ? BLACK : WHITE;

      token = char(toupper(token));

      if (token == 'K')
          for (rsq = relative_square(c, H1); pieceTypeOf(piece_on(rsq)) != ROOK; rsq=Square(rsq-1)) {}

      else if (token == 'Q')
          for (rsq = relative_square(c, A1); pieceTypeOf(piece_on(rsq)) != ROOK; rsq=Square(rsq+1)) {}

      else if (token >= 'A' && token <= 'H')
          rsq = make_square(File(token - 'A'), relative_rank(c, RANK_1));

      else
          continue;

      set_castle_right(c, rsq);
  }

  // 4. En passant square. D.A.G., 03/05/2012: Used to ignore if no pawn capture is possible
  if (   ((fen >> col) && (col >= 'a' && col <= 'h'))
      && ((fen >> row) && (row == '3' || row == '6')))
  {
      st->epSquare = make_square(File(col - 'a'), Rank(row - '1'));
	  put_piece(EN_PASSANT, st->epSquare); // Added by D.A.G. on 03/05/2012
	  // En passant square in FEN is recorded regardless of whether
	  // there is a pawn in position to make an en passant capture.
	  // D.A.G., 03/05/2012: Commented out the code below that used to ignore
	  // en passant info from FEN if no pawn capture were possible.
	  // Reason: I don't want attackers_to() to be invoked so early.
	  /*
      if ((attackers_to(st->epSquare) & pieces(PAWN, sideToMove)).none())
          st->epSquare = NO;
	  */
  }

  // 5-6. Halfmove clock and fullmove number
  fen >> std::skipws >> st->rule50 >> startPosPly;

  // Convert from fullmove starting from 1 to ply starting from 0,
  // handle also common incorrect FEN with fullmove = 0.
  startPosPly = std::max(2 * (startPosPly - 1), 0) + int(sideToMove == BLACK);

  st->key = compute_key();
  // D.A.G., 02/27/2012: Comment out computation of pawn key and material key:
  /*
  st->pawnKey = compute_pawn_key();
  st->materialKey = compute_material_key();
  */
  // D.A.G., 02/27/2012: Comment out value computation:
  /*
  st->value = compute_value();
   */
  // The non-pawn material computation is needed for is_draw()
  st->npMaterial[WHITE] = compute_non_pawn_material(WHITE);
  st->npMaterial[BLACK] = compute_non_pawn_material(BLACK);

  // D.A.G., 03/05/2012: Comment out the checkers computation:
  /*
  st->checkersBB = attackers_to(king_square(sideToMove)) & pieces(~sideToMove);
  */
  chess960 = isChess960;

  // D.A.G. commented out the assert:
  //assert(pos_is_ok());
}


/// Position::set_castle_right() is an helper function used to set castling
/// rights given the corresponding color and the rook starting square.

void Position::set_castle_right(Color c, Square rsq)
{
  int f = (rsq < king_square(c) ? WHITE_OOO : WHITE_OO) << c;

  st->castleRights |= f;
  castleRightsMask[king_square(c)] ^= f;
  castleRightsMask[rsq] ^= f;
  castleRookSquare[f] = rsq;
}


/// Position::to_fen() returns a FEN representation of the position. In case
/// of Chess960 the Shredder-FEN notation is used. Mainly a debugging function.

const string Position::to_fen() const
{
  std::ostringstream fen;
  Square sq;
  int emptyCnt;

  for (Rank rank = RANK_8; rank >= RANK_1; rank=Rank(rank-1))
  {
      emptyCnt = 0;

      for (File file = FILE_A; file <= FILE_H; file=File(file+1))
      {
          sq = make_square(file, rank);

          if (square_is_empty(sq))
              emptyCnt++;
          else
          {
              if (emptyCnt > 0)
              {
                  fen << emptyCnt;
                  emptyCnt = 0;
              }
              fen << PieceToChar[piece_on(sq)];
          }
      }

      if (emptyCnt > 0)
          fen << emptyCnt;

      if (rank > RANK_1)
          fen << '/';
  }

  fen << (sideToMove == WHITE ? " w " : " b ");

  if (can_castle(WHITE_OO))
      fen << (chess960 ? char(toupper(file_to_char(file_of(castle_rook_square(WHITE_OO))))) : 'K');

  if (can_castle(WHITE_OOO))
      fen << (chess960 ? char(toupper(file_to_char(file_of(castle_rook_square(WHITE_OOO))))) : 'Q');

  if (can_castle(BLACK_OO))
      fen << (chess960 ? file_to_char(file_of(castle_rook_square(BLACK_OO))) : 'k');

  if (can_castle(BLACK_OOO))
      fen << (chess960 ? file_to_char(file_of(castle_rook_square(BLACK_OOO))) : 'q');

  if (st->castleRights == CASTLES_NONE)
      fen << '-';

  fen << (ep_square() == NO ? " - " : " " + square_to_string(ep_square()) + " ")
      << st->rule50 << " " << 1 + (startPosPly - int(sideToMove == BLACK)) / 2;

  return fen.str();
}

/// move_to_uci() converts a move to a string in coordinate notation
/// (g1f3, a7a8q, etc.). The only special case is castling moves, where we
/// print in the e1g1 notation in normal chess mode, and in e1h1 notation in
/// Chess960 mode. [D.A.G. disabled the Chess960 mode.]

const string move_to_uci(Move m, bool chess960)
{
  Square from = from_sq(m);
  Square to = to_sq(m);
  string promotion;

  if (m.moveFlags[M_MOVE_NONE])
      return "(none)";

  if (m.moveFlags[M_NULL_MOVE])
      return "0000";
  // D.A.G., 03/05/2012: I commented out the code for handling of castling.
  // Internally, the castling move used to be coded as "king captures rook".
  // This was needed for chess960, which we don't support.
  /*
  if (is_castle(m) && !chess960)
      to = Square(from + (file_of(to) == FILE_H ? Square(2) : -Square(2)));
  */

  if (is_promotion(m))
      promotion = char(tolower(piece_type_to_char(m.promotionPieceType)));

  return square_to_string(from) + square_to_string(to) + promotion;
}

/// Position::print() prints an ASCII representation of the position to
/// the standard output. If a move were given then also the san (standard algebraic notation)
/// used to be printed. D.A.G., 03/05/2012: Replaced the latter feature with a call to move_to_uci.

void Position::print(Move m) const
{
  const char* dottedLine = "\n+---+---+---+---+---+---+---+---+\n";

  Position p(*this, thread()); // D.A.G., 03/05/2012: A copy-constructor is invoked here!
  cout << "\nMove is: " << (sideToMove == BLACK ? ".." : "") << move_to_uci(m, chess960!=0);
  // D.A.G., 03/05/2012: Replaced move_to_san(p, move); with a call to move_to_uci above.

  for (Rank rank = RANK_8; rank >= RANK_1; rank=Rank(rank-1))
  {
	  bool ep_flag;
      cout << dottedLine << '|';
      for (File file = FILE_A; file <= FILE_H; file=File(file+1))
      {
          Square sq = make_square(file, rank);
          Piece piece = piece_on(sq);
		  if (piece == EN_PASSANT)
			  ep_flag=true;
		  else
			  ep_flag=false;
          char c = (colorOf(piece) == BLACK ? '=' : ' ');

          if (piece == NO_PIECE && !opposite_colors(sq, A1))
              piece=Piece(piece+1); // Index the dot

          cout << c << (ep_flag?'e':PieceToChar[piece]) << c << '|';
      }
  }
  cout << dottedLine << "Fen is: " << to_fen() << "\nKey is: " << st->key << endl;
}

/// Print a move
void Position::print_move(Move m, int i) const
{
	cout << "Move " << i << " is: " << (sideToMove == BLACK ? ".." : "") << move_to_uci(m,chess960!=0) << endl;
}

/// Print the move's flags, for debugging
void Position::print_flags(Move m) const
{
	if (m.moveFlags[M_EN_PASSANT])
		cout << "M_EN_PASSANT" << endl;
	if (m.moveFlags[M_CASTLE_SHORT])
		cout << "M_CASTLE_SHORT" << endl;
	if (m.moveFlags[M_CASTLE_LONG])
		cout << "M_CASTLE_LONG" << endl;
	if (m.moveFlags[M_NULL_MOVE])
		cout << "M_NULL_MOVE" << endl;
	if (m.moveFlags[M_MOVE_NONE])
		cout << "M_MOVE_NONE" << endl;
	if (m.moveFlags[M_BOOK_MOVE])
		cout << "M_BOOK_MOVE" << endl;
	if (m.moveFlags[M_CAPTURE])
		cout << "M_CAPTURE" << endl;
	if (m.moveFlags[M_ISCHECK])
		cout << "M_ISCHECK" << endl;
	if (m.moveFlags[M_KILLER])
		cout << "M_KILLER" << endl;
	if (m.moveFlags[M_PASSAGE])
		cout << "M_PASSAGE" << endl;
	if (m.moveFlags[M_CHECK_DEFENSE])
		cout << "M_CHECK_DEFENSE" << endl;
	if (m.moveFlags[M_EVASION])
		cout << "M_EVASION" << endl;
	if (m.moveFlags[M_QUIET])
		cout << "M_QUIET" << endl;
	if (m.moveFlags[M_DRAW])
		cout << "M_DRAW" << endl;
	if (m.moveFlags[M_STALEMATE])
		cout << "M_STALEMATE" << endl;
	if (m.moveFlags[M_CHECKMATE])
		cout << "M_CHECKMATE = " << endl;
}

/// Position::clear() erases the position object to a pristine state, with an
/// empty board, white to move, and no castling rights.
void Position::clear()
{
  st = &startState;
  memset(st, 0, sizeof(StateInfo));
  st->epSquare = NO;

  memset(byColorBB,  0, sizeof(Bitboard) * NUM_COLORS);
  memset(byTypeBB,   0, sizeof(Bitboard) * NUM_PIECE_TYPES);
  memset(pieceCount, 0, sizeof(int) * NUM_COLORS * NUM_PIECE_TYPES);
  memset(index,      0, sizeof(int) * NUM_SQUARES);

  for (int i = 0; i < NUM_PIECE_TYPES; i++)
      for (int j = 0; j < NUM_SQUARES; j++)
          pieceList[0][i][j] = pieceList[1][i][j] = NO;

  for (Square sq = A1; sq <= H8; sq=Square(sq+1))
  {
      board[sq] = NO_PIECE;
      castleRightsMask[sq] = ALL_CASTLES;
  }
  sideToMove = WHITE;
  nodes = 0;
  occupied = 0;
}

/// Position::put_piece() puts a piece on the given square of the board,
/// updating the board array, pieces list, bitboards, and piece counts.
void Position::put_piece(Piece p, Square s)
{
  Color c = colorOf(p);
  PieceType pt = pieceTypeOf(p);

  // D.A.G., 03/08/2012: Added the if statement to handle en passant as piece
  if (c==NO_COLOR)
	  c = ~sideToMove;
  board[s] = p;
  index[s] = pieceCount[c][pt]++;
  pieceList[c][pt][index[s]] = s;

  byTypeBB[pt] |= s;
  byColorBB[c] |= s;
  occupied |= s;
}


/// Position::compute_key() computes the hash key of the position. The hash
/// key is usually updated incrementally as moves are made and unmade, the
/// compute_key() function is only used when a new position is set up, and
/// to verify the correctness of the hash key when running in debug mode.

Key Position::compute_key() const
{
  Key result = zobCastle[st->castleRights];

  for (Square s = A1; s <= H8; s=Square(s+1))
      if (!square_is_empty(s))
          result ^= zobrist[piece_on(s)][s];
  // D.A.G., 03/05/2012: Commented out old computation for en passant below;
  // EN_PASSANT is now a piece, handled above.
  /*
  if (ep_square() != SQ_NONE)
      result ^= zobEp[ep_square()];
  */
  if (sideToMove == BLACK)
      result ^= zobSideToMove;

  return result;
}

/// Position::compute_non_pawn_material() computes the total non-pawn middle
/// game material value for the given side. Material values are updated
/// incrementally during the search, this function is only used while
/// initializing a new Position object.

int Position::compute_non_pawn_material(Color c) const
{
  int result = 0;

  for (PieceType pt = PAWN; pt <= BISHOP; pt=PieceType(pt+1))
      result += piece_count(c, pt) * PieceValueMidgame[pt];

  return result;
}

/// Position::is_draw() tests whether the position is drawn by material,
/// repetition, or the 50 moves rule. It does not detect stalemates, that
/// is done at the end of RootMoveList::generate_from_position() in movegen.cpp.
template<bool SkipRepetition>
bool Position::is_draw() const
{
  // Draw by material?
  if (pieces(PAWN).none() &&
	  (non_pawn_material(WHITE) + non_pawn_material(BLACK) <= BishopValueMidgame))
	  // BishopValueMidgame == 840 (7 of Kaissa 1.00 multiplied by 120), see chess.h
      return true;

  // Draw by the 50 moves rule?       /* D.A.G., 03/19/2012. Commented out. */
  if (st->rule50 > 99 /*&& (!in_check()*/ /*|| MoveList<MV_LEGAL>(*this).size())*/)
	  // D.A.G., 03/19/2012. The purpose of !in_check() was to eventually make sure
	  // that we're not checkmated when we're trying to claim the draw. Note that
	  // checkersBB has to be computed by the CheckInfo constructor before in_check()
	  // can work correctly!
      return true;

  // Draw by repetition?
  if (!SkipRepetition)
  {
      int i = 4, e = std::min(st->rule50, st->pliesFromNull);

      if (i <= e)
      {
          StateInfo* stp = st->previous->previous;

          do {
              stp = stp->previous->previous;

              if (stp->key == st->key)
                  return true;

              i +=2;

          } while (i <= e);
      }
  }

  return false;
}

// Explicit template instantiations
template bool Position::is_draw<false>() const;
template bool Position::is_draw<true>() const;

/// Position::attackers_to() computes a bitboard of all pieces which attack a
/// given square. Slider attacks use occ bitboard as occupancy.
// D.A.G., 03/05/2012: Comment out attackers_to for the time being.
/*
Bitboard Position::attackers_to(Square s, Bitboard occ) const {

  return  (attacks_from<PAWN>(s, BLACK) & pieces(PAWN, WHITE))
        | (attacks_from<PAWN>(s, WHITE) & pieces(PAWN, BLACK))
        | (attacks_from<KNIGHT>(s)      & pieces(KNIGHT))
        | (rook_attacks_bb(s, occ)      & pieces(ROOK, QUEEN))
        | (bishop_attacks_bb(s, occ)    & pieces(BISHOP, QUEEN))
        | (attacks_from<KING>(s)        & pieces(KING));
}
*/

// CheckInfo parameterized constructor
// Detect their checks and determine our pinned pieces:
CheckInfo::CheckInfo(Position& pos, Color sideToMove, bool& lookForCheckDefense)
{
	Piece  p, p_our, p_our_too, p_their, p_their_too/*, p_candidate_for_pinned*/;
	Square s, candidate_for_pinned_s, enemy_ksq;
	int    dir, opt;

	// Where is the king of the side to move ("our side", "us")?
	ksq = pos.king_square(sideToMove);
	// Initialize checkersBB to all zeros:
	pos.clear_checkers();
	// Initialize dcCandidates, pinned, checkSq[NUM_PIECE_TYPES], and lineOfDefense to all zeros:
	dcCandidates.reset();
	pinned.reset();
	for(PieceType pt=NO_PIECE_TYPE;pt<=BISHOP;pt=PieceType(pt+1))
		checkSq[pt].reset();
	// initialize the line of defense to all NO's:
	for (opt = 0; opt < MAX_CHECK_DEFENSE_SQUARES; opt++)
		lineOfDefense[opt] = NO;

	/// Are we in check from an enemy PAWN?..
	// if we're playing white, imagine a white pawn in the white king's square;
	// otherwise imagine a black pawn in the black king's square.
	// Now explore the pawn's capture directions (1 and 2), with at most one
	// option each, to find out if a pawn of the opposite color is there:
	p_our = Piece(WHITE_PAWN + sideToMove);
	p_their = Piece(WHITE_PAWN + ~sideToMove);
	dir=1;
    while (protoMoves[p_our][ksq][dir][0]!=NO) // with pawns, we won't exhaust directions
	{
		opt=0;
		do
		{
			s = protoMoves[p_our][ksq][dir][opt];
			if(pos.piece_on(s)==p_their)     // found a pawn checker
			{
				// Enter the checker info into the StateInfo struct:
				pos.set_checker_square(s);
				// Enter the checker info into the CheckInfo struct:
				// checkSq[PAWN][s] = 1;
				// Cannot defend against a pawn check:
				lookForCheckDefense = false;
			}
			opt++;
		}
		while (protoMoves[p_our][ksq][dir][opt]!=NO);
		dir++;
	}

	/// Are we in check from an enemy KNIGHT?
	// if we're playing white, imagine a white knight in the white king's square;
	// otherwise imagine a black knight in the black king's square.
	// Now explore the knight's capture directions, with at most one
	// option each, to find out if a knight of the opposite color is there:
	p_our = Piece(WHITE_KNIGHT + sideToMove);
	p_their = Piece(WHITE_KNIGHT + ~sideToMove);
	dir=0;
    while ((dir<MAX_DIRECTIONS)&&(protoMoves[p_our][ksq][dir][0]!=NO))
	{
		opt=0;
		do
		{
			s = protoMoves[p_our][ksq][dir][opt];
			if(pos.piece_on(s)==p_their)
			{
				// Enter the checker info into the StateInfo struct:
				pos.set_checker_square(s);
				// Enter the checker info into the CheckInfo struct:
				// checkSq[KNIGHT][s] = 1;
				// Cannot defend against a knight check:
				lookForCheckDefense = false;
			}
			opt++;
		}
		while (protoMoves[p_our][ksq][dir][opt]!=NO);
		dir++;
	}

	/// Are we in check from an enemy QUEEN or ROOK
	/// from a horizontal or vertical direction?
	// if we're playing white, imagine a white rook in the white king's square;
	// otherwise imagine a black rook in the black king's square.
	// Now explore the rook's capture directions to find out if
	// a queen or rook of the opposite color is there:
	p_our = Piece(WHITE_ROOK + sideToMove);
	p_their = Piece(WHITE_ROOK + ~sideToMove);
	p_their_too = Piece(WHITE_QUEEN + ~sideToMove);
	dir=0;
    while (protoMoves[p_our][ksq][dir][0]!=NO) // with rooks, we won't exhaust directions
	{
		opt=0;
		// Won't look for a hidden checker until a pinned piece is encountered:
		bool lookForHiddenChecker = false;
		do
		{
			s = protoMoves[p_our][ksq][dir][opt];
			p = pos.piece_on(s);
			if (p > EN_PASSANT) // if the square is NOT (empty OR en_passant)
			{
				if (colorOf(p)==sideToMove) // found our piece
				{
					if (lookForHiddenChecker)
						break; // There is no hidden checker
					else
					{   // Start looking for a hidden checker
						lookForHiddenChecker = true;
						candidate_for_pinned_s = s;
						//p_candidate_for_pinned = p;
					}
				}
				else if (p==p_their) // found an enemy rook
				{
					if (lookForHiddenChecker)
					{   // found a hidden checker
						// Enter the hidden checker info into dcCandidates:
						// dcCandidates[s] = 1;
						// Update the bitboard of pinned pieces:
						pinned[candidate_for_pinned_s] = 1;
						break;
					}
					else
					{   // found a checker
						if ((!opt) || pos.how_many_checkers())
							// Cannot defend against a check from an adjacent square or
							// against multiple checkers:
							lookForCheckDefense = false;
						else
						{
							// Set the line of defense:
							for (int i=0;i<opt;i++)
								lineOfDefense[i]=protoMoves[p_our][ksq][dir][i];
						}
						// Enter the checker info into the StateInfo struct:
						pos.set_checker_square(s);
						// Enter the checker info into the CheckInfo struct:
						//checkSq[ROOK][s] = 1;
						break;
					}
				}
				else if (p==p_their_too) // found an enemy queen
				{
					if (lookForHiddenChecker)
					{   // found a hidden checker
						// Enter the hidden checker info into dcCandidates:
						// dcCandidates[s] = 1;
						// Update the bitboard of pinned pieces:
						pinned[candidate_for_pinned_s] = 1;
						break;
					}
					else
					{   // found a checker
						if ((!opt) || pos.how_many_checkers())
							// Cannot defend against a check from an adjacent square or
							// against multiple checkers:
							lookForCheckDefense = false;
						else
						{
							// Set the line of defense:
							for (int i=0;i<opt;i++)
								lineOfDefense[i]=protoMoves[p_our][ksq][dir][i];
						}
						// Enter the checker info into the StateInfo struct:
						pos.set_checker_square(s);
						// Enter the checker info into the CheckInfo struct:
						//checkSq[QUEEN][s] = 1;
						break;
					}
				}
				else break; // found a non-checking enemy piece
			}
			opt++;
		}
		while (protoMoves[p_our][ksq][dir][opt]!=NO);
		dir++;
	}

	/// Are we in check from an enemy QUEEN or BISHOP from a diagonal direction?
	// If we're playing white, imagine a white bishop in the white king's square;
	// otherwise imagine a black bishop in the black king's square.
	// Now explore the bishop's capture directions to find out if
	// a queen or bishop of the opposite color is there:
	p_our = Piece(WHITE_BISHOP + sideToMove);
	p_their = Piece(WHITE_BISHOP + ~sideToMove);
	p_their_too = Piece(WHITE_QUEEN + ~sideToMove);
	dir=0;
    while (protoMoves[p_our][ksq][dir][0]!=NO) // with bishops, we won't exhaust directions
	{
		opt=0;
		// Won't look for a hidden checker until a pinned piece is encountered:
		bool lookForHiddenChecker = false;
		do
		{
			s = protoMoves[p_our][ksq][dir][opt];
			p = pos.piece_on(s);
			if (p > EN_PASSANT) // if the square is NOT (empty OR en_passant)
			{
				if (colorOf(p)==sideToMove) // found our piece
				{
					if (lookForHiddenChecker)
						break; // There is no hidden checker
					else
					{   // Start looking for a hidden checker
						lookForHiddenChecker = true;
						candidate_for_pinned_s = s;
						//p_candidate_for_pinned = p;
					}
				}
				else if (p==p_their) // found an enemy bishop
				{
					if (lookForHiddenChecker)
					{   // found a hidden checker
						// Enter the hidden checker info into dcCandidates:
						// dcCandidates[s] = 1;
						// Update the bitboard of pinned pieces:
						pinned[candidate_for_pinned_s] = 1;
						break;
					}
					else
					{   // found a checker
						if ((!opt) || pos.how_many_checkers())
							// Cannot defend against a check from an adjacent square or
							// against multiple checkers:
							lookForCheckDefense = false;
						else
						{
							// Set the line of defense:
							for (int i=0;i<opt;i++)
								lineOfDefense[i]=protoMoves[p_our][ksq][dir][i];
						}
						// Enter the checker info into the StateInfo struct:
						pos.set_checker_square(s);
						// Enter the checker info into the CheckInfo struct:
						//checkSq[BISHOP][s] = 1;
						break;
					}
				}
				else if (p==p_their_too) // found an enemy queen
				{
					if (lookForHiddenChecker)
					{   // found a hidden checker
						// Enter the hidden checker info into dcCandidates:
						// dcCandidates[s] = 1;
						// Update the bitboard of pinned pieces:
						pinned[candidate_for_pinned_s] = 1;
						break;
					}
					else
					{   // found a checker
						if ((!opt) || pos.how_many_checkers())
							// Cannot defend against a check from an adjacent square or
							// against multiple checkers:
							lookForCheckDefense = false;
						else
						{
							// Set the line of defense:
							for (int i=0;i<opt;i++)
								lineOfDefense[i]=protoMoves[p_our][ksq][dir][i];
						}
						// Enter the checker info into the StateInfo struct:
						pos.set_checker_square(s);
						// Enter the checker info into the CheckInfo struct:
						//checkSq[QUEEN][s] = 1;
						break;
					}
				}
				else break; // found a non-checking enemy piece
			}
			opt++;
		}
		while (protoMoves[p_our][ksq][dir][opt]!=NO);
		dir++;
	}

	/// Compute (a) checkSq: Bitboards of squares that our piece of a given
	/// piece type can give a check from, and (b) our candidates for discovered check

	// Where is the king of the opposite side ("their side", "them")?
	enemy_ksq = pos.king_square(~sideToMove);

	// Explore pawn options.
	// In place of the enemy king, imagine a pawn of the same color.
	// Its capture options (directions 1 and 2) show spots from where
	// the enemy king could be given check by our pawn.
	p_their = Piece(WHITE_PAWN + ~sideToMove);
	dir=1;
	while ((s=protoMoves[p_their][enemy_ksq][dir][0])!=NO) // with pawns, we won't exhaust directions
	{
		p = pos.piece_on(s);
		if (p > EN_PASSANT) // if the square is NOT (empty OR en_passant) and contains their piece
		{
			if (colorOf(p)==~sideToMove) // found their piece
				checkSq[PAWN][s]=1;
		}
		else
			checkSq[PAWN][s]=1;
		dir++;
	}

	// Explore knight options.
	// In place of the enemy king, imagine a knight of the same color.
	// Its move options show spots from where
	// the enemy king could be given check by our knight.
	p_their = Piece(WHITE_KNIGHT + ~sideToMove);
	dir=0;
	while ((dir < MAX_DIRECTIONS)&&((s=protoMoves[p_their][enemy_ksq][dir][0])!=NO))
	{
		p = pos.piece_on(s);
		if (p > EN_PASSANT) // if the square is NOT (empty OR en_passant) and contains their piece
		{
			if (colorOf(p)==~sideToMove) // found their piece
				checkSq[KNIGHT][s]=1;
		}
		else
			checkSq[KNIGHT][s]=1;
		dir++;
	}

	// Explore rook/queen horizontal and vertical options
	p_their = Piece(WHITE_ROOK + ~sideToMove);
	p_our = Piece(WHITE_ROOK + sideToMove);
	p_our_too = Piece(WHITE_QUEEN + sideToMove);
	dir=0;
    while (protoMoves[p_their][enemy_ksq][dir][0]!=NO) // with rooks, we won't exhaust directions
	{
		opt=0;
		// Won't look for a hidden checker until our non-checking piece is encountered:
		bool lookForHiddenChecker = false;
		do
		{
			s = protoMoves[p_their][enemy_ksq][dir][opt];
			p = pos.piece_on(s);
			if (p > EN_PASSANT) // if the square is NOT (empty OR en_passant)
			{
				if (colorOf(p)==~sideToMove) // found their piece
				{
					if(!lookForHiddenChecker)
						checkSq[ROOK][s]=checkSq[QUEEN][s]=1;
					break; // cannot use it for a discovered check,
				           // unless the piece is a pawn that can be taken en passant;
				           // we'll handle that special case separately!
				}
				else if ((p==p_our)||(p==p_our_too)) // found our rook or queen,
				{   // which must be a HIDDEN checker
					// Enter the discovered check candidate info into dcCandidates:
					if (lookForHiddenChecker)
						dcCandidates[candidate_for_pinned_s] = 1;
					break;
				}
				else // found our non-checking piece
				{
					if (lookForHiddenChecker)
						break; // There is no hidden checker
					else // Start looking for a hidden checker
					{
						lookForHiddenChecker = true;
						candidate_for_pinned_s = s;
					}
				}
			}
			else if (!lookForHiddenChecker)
				checkSq[ROOK][s]=checkSq[QUEEN][s]=1;
			opt++;
		}
		while (protoMoves[p_their][enemy_ksq][dir][opt]!=NO);
		dir++;
	}
	/// Continue the computation of our candidates for discovered check
	// Explore bishop/queen diagonal options
	p_their = Piece(WHITE_BISHOP + ~sideToMove);
	p_our = Piece(WHITE_BISHOP + sideToMove);
	p_our_too = Piece(WHITE_QUEEN + sideToMove);
	dir=0;
    while (protoMoves[p_their][enemy_ksq][dir][0]!=NO) // with bishops, we won't exhaust directions
	{
		opt=0;
		// Won't look for a hidden checker until our non-checking piece is encountered:
		bool lookForHiddenChecker = false;
		do
		{
			s = protoMoves[p_their][enemy_ksq][dir][opt];
			p = pos.piece_on(s);
			if (p > EN_PASSANT) // if the square is NOT (empty OR en_passant)
			{
				if (colorOf(p)==~sideToMove) // found their piece
				{
					if(!lookForHiddenChecker)
						checkSq[BISHOP][s]=checkSq[QUEEN][s]=1;
					break; // cannot use it for a discovered check,
				           // unless the piece is a pawn that can be taken en passant;
				           // we'll handle that special case separately!
				}
				else if ((p==p_our)||(p==p_our_too)) // found our bishop or queen,
				{   // which must be a HIDDEN checker
					// Enter the discovered check candidate info into dcCandidates:
					if (lookForHiddenChecker)
						dcCandidates[candidate_for_pinned_s] = 1;
					break;
				}
				else // found our non-checking piece
				{
					if (lookForHiddenChecker)
						break; // There is no hidden checker
					else // Start looking for a hidden checker
					{
						lookForHiddenChecker = true;
						candidate_for_pinned_s = s;
					}
				}
			}
			else if (!lookForHiddenChecker)
				checkSq[BISHOP][s]=checkSq[QUEEN][s]=1;
			opt++;
		}
		while (protoMoves[p_their][enemy_ksq][dir][opt]!=NO);
		dir++;
	}

	/// Output for debugging:
	pos.print_checkers();         // Print the checkers bitboard
	print_bitboard(dcCandidates, "Discovered check candidates:"); // Print the dcCandidates bitboard
	print_bitboard(pinned, "Pinned pieces:");       // Print the pinned pieces bitboard
	Bitboard lineOfDefenseBB;     // the line of defense bitboard
	// Initialize the line of defense bitboard:
	lineOfDefenseBB.reset();      // clears every bit
	// Fill the line of defense bitboard:
	opt = 0;
	while(lineOfDefense[opt]!=NO)
	{
		lineOfDefenseBB[lineOfDefense[opt]] = 1;
		opt++;
	}
	// Print the line of defense bitboard:
	print_bitboard(lineOfDefenseBB, "Line of defense:");

	//print_bitboard(checkSq[ROOK],"checkSq[ROOK]");
}

Bitboard Position::attackers_to(Square s, Square ksq, Color sideToMove) const
{
	Bitboard attackers;
	Piece p, p_their, p_our, p_our_too;
	Square sq;
	int dir, opt;

	// Initialize the attackers bitboard with all zeros:
	attackers.reset();

	/// Are we attacking Square s with a PAWN?..
	// If we're playing white, imagine a black pawn in Square s;
	// otherwise imagine a white pawn in the square.
	// Now explore the imaginary pawn's capture directions (1 and 2), with at most one
	// option each, to find out if a pawn of our color is there:
	p_our = Piece(WHITE_PAWN + sideToMove);
	p_their = Piece(WHITE_PAWN + ~sideToMove);
	dir=1;
    while (protoMoves[p_their][s][dir][0]!=NO) // with pawns, we won't exhaust directions
	{
		opt=0;
		do
		{
			sq = protoMoves[p_their][s][dir][opt];
			if(piece_on(sq)==p_our)     // found a pawn attacker
			{
				// Enter the attacker info into the attackers bitboard:
				attackers[sq]=1;
			}
			opt++;
		}
		while (protoMoves[p_their][s][dir][opt]!=NO);
		dir++;
	}

	/// Are we attacking Square s with a KNIGHT?
	// if we're playing white, imagine a black knight in Square s;
	// otherwise imagine a white knight in the square.
	// Now explore the knight's capture directions, with at most one
	// option each, to find out if a knight of our color is there:
	p_our = Piece(WHITE_KNIGHT + sideToMove);
	p_their = Piece(WHITE_KNIGHT + ~sideToMove);
	dir=0;
    while ((dir<MAX_DIRECTIONS)&&(protoMoves[p_their][s][dir][0]!=NO))
	{
		opt=0;
		do
		{
			sq = protoMoves[p_their][s][dir][opt];
			if(piece_on(sq)==p_our) // found a knight attacker
			{
				// Enter the attacker info into the attackers bitboard:
				attackers[sq]=1;
			}
			opt++;
		}
		while (protoMoves[p_their][s][dir][opt]!=NO);
		dir++;
	}

	/// Are we attacking Square s with a QUEEN or ROOK
	/// from a horizontal or vertical direction?
	// if we're playing white, imagine a black rook in Square s;
	// otherwise imagine a white rook in the square.
	// Now explore the rook's capture directions to find out if
	// a queen or rook of our color is there:
	p_our = Piece(WHITE_ROOK + sideToMove);
	p_their = Piece(WHITE_ROOK + ~sideToMove);
	p_our_too = Piece(WHITE_QUEEN + sideToMove);
	dir=0;
    while (protoMoves[p_their][s][dir][0]!=NO) // with rooks, we won't exhaust directions
	{
		opt=0;
		do
		{
			sq = protoMoves[p_their][s][dir][opt];
			p = piece_on(sq);
			if (p > EN_PASSANT) // if the square is NOT (empty OR en_passant)
			{
				if ((p==p_our)||(p==p_our_too)) // found our rook or queen
				{
					// Enter the attacker info into the attackers bitboard:
					attackers[sq]=1;
				}
				break;
			}
			opt++;
		}
		while (protoMoves[p_their][s][dir][opt]!=NO);
		dir++;
	}

	/// Are we attacking Square s with a QUEEN or BISHOP from a diagonal direction?
	// If we're playing white, imagine a black bishop in Square s;
	// otherwise imagine a white bishop in the square.
	// Now explore the bishop's capture directions to find out if
	// a queen or bishop of our color is there:
	p_our = Piece(WHITE_BISHOP + sideToMove);
	p_their = Piece(WHITE_BISHOP + ~sideToMove);
	p_our_too = Piece(WHITE_QUEEN + sideToMove);
	dir=0;
    while (protoMoves[p_their][s][dir][0]!=NO) // with bishops, we won't exhaust directions
	{
		opt=0;
		do
		{
			sq = protoMoves[p_their][s][dir][opt];
			p = piece_on(sq);
			if (p > EN_PASSANT) // if the square is NOT (empty OR en_passant)
			{
				if ((p==p_our)||(p==p_our_too)) // found our bishop or queen
				{
					// Enter the attacker info into the attackers bitboard:
					attackers[sq]=1;
				}
				break;
			}
			opt++;
		}
		while (protoMoves[p_their][s][dir][opt]!=NO);
		dir++;
	}

	/// Are we attacking Square s with our KING?
	// We will only add the king square to attackers if Square s
	// is not defended by the opposite side:
	if ((file_distance(s,ksq)<2)&&(rank_distance(s,ksq)<2))
	{
		if (not_defended(s,~sideToMove))
			attackers[ksq]=1;
	}
	return attackers;
}

bool Position::not_defended(Square s, Color sideToMove) const
{
	bool notDefended=true;
	Piece p, p_their, p_their_too, p_our, p_our_too;
	Square sq;
	int dir, opt;

	/// Are we defending Square s with a PAWN?..
	// if we're playing white, imagine a black pawn in Square s;
	// otherwise imagine a white pawn in the square.
	// Now explore the imaginary pawn's capture directions (1 and 2), with at most one
	// option each, to find out if a pawn of our color is there:
	p_our = Piece(WHITE_PAWN + sideToMove);
	p_their = Piece(WHITE_PAWN + ~sideToMove);
	dir=1;
    while (protoMoves[p_their][s][dir][0]!=NO) // with pawns, we won't exhaust directions
	{
		opt=0;
		do
		{
			sq = protoMoves[p_their][s][dir][opt];
			if(piece_on(sq)==p_our)     // found a pawn defender
				return false; // notDefended == false
			opt++;
		}
		while (protoMoves[p_their][s][dir][opt]!=NO);
		dir++;
	}

	/// Are we defending Square s with a KNIGHT?
	// if we're playing white, imagine a black knight in Square s;
	// otherwise imagine a white knight in the square.
	// Now explore the knight's capture directions, with at most one
	// option each, to find out if a knight of our color is there:
	p_our = Piece(WHITE_KNIGHT + sideToMove);
	p_their = Piece(WHITE_KNIGHT + ~sideToMove);
	dir=0;
    while ((dir<MAX_DIRECTIONS)&&(protoMoves[p_their][s][dir][0]!=NO))
	{
		sq = protoMoves[p_their][s][dir][0];
		if(piece_on(sq)==p_our) // found a knight defender
			return false; // notDefended == false
		dir++;
	}

	/// Are we defending Square s with a QUEEN or ROOK
	/// from a horizontal or vertical direction?
	// if we're playing white, imagine a black rook in Square s;
	// otherwise imagine a white rook in the square.
	// Now explore the rook's capture directions to find out if
	// a queen or rook of our color is there:
	p_our = Piece(WHITE_ROOK + sideToMove);
	p_their = Piece(WHITE_ROOK + ~sideToMove);
	p_our_too = Piece(WHITE_QUEEN + sideToMove);
	p_their_too = Piece(WHITE_KING + ~sideToMove);
	dir=0;
    while (protoMoves[p_their][s][dir][0]!=NO) // with rooks, we won't exhaust directions
	{
		opt=0;
		do
		{
			sq = protoMoves[p_their][s][dir][opt];
			p = piece_on(sq);
			if (p > EN_PASSANT) // if the square is NOT (empty OR en_passant)
			{
				if ((p==p_our)||(p==p_our_too)) // found our rook or queen
					return false; // notDefended == false
				else if (p!=p_their_too) // we can "see through" the enemy king!
					break;
			}
			opt++;
		}
		while (protoMoves[p_their][s][dir][opt]!=NO);
		dir++;
	}

	/// Are we defending Square s with a QUEEN or BISHOP from a diagonal direction?
	// If we're playing white, imagine a black bishop in Square s;
	// otherwise imagine a white bishop in the square.
	// Now explore the bishop's capture directions to find out if
	// a queen or bishop of our color is there:
	p_our = Piece(WHITE_BISHOP + sideToMove);
	p_their = Piece(WHITE_BISHOP + ~sideToMove);
	p_our_too = Piece(WHITE_QUEEN + sideToMove);
	dir=0;
    while (protoMoves[p_their][s][dir][0]!=NO) // with bishops, we won't exhaust directions
	{
		opt=0;
		do
		{
			sq = protoMoves[p_their][s][dir][opt];
			p = piece_on(sq);
			if (p > EN_PASSANT) // if the square is NOT (empty OR en_passant)
			{
				if ((p==p_our)||(p==p_our_too)) // found our bishop or queen
					return false; // notDefended == false
				else if (p!=p_their_too) // we can "see through" the enemy king!
					break;
			}
			opt++;
		}
		while (protoMoves[p_their][s][dir][opt]!=NO);
		dir++;
	}

	/// Are we defending Square s with our KING?
	Square ksq=king_square(sideToMove);
	if ((file_distance(s,ksq)<2)&&(rank_distance(s,ksq)<2)) // We assume s!=ksq
		return false; // notDefended == false
	return true; // notDefended == true
}

/// defenders_of finds out which of our pieces can defend our king
/// against a check by moving to Square dsq of the line of defense:
Bitboard Position::defenders_of(Square dsq, Color sideToMove) const
{
	Bitboard defenders;
	Piece p, p_their, p_our, p_our_too;
	Square sq;
	Rank r;
	int dir, opt;

	// Initialize the defenders bitboard with all zeros:
	defenders.reset();

	/// Can we move a PAWN to Square dsq?..
	if (sideToMove)
	{   // black's turn to move
		r = ranksOfSquares[dsq];
		if(r==RANK_5)
		{   // find out if a black pawn can move by two squares
			sq = Square(dsq+16);
			if ((piece_on(sq)==BLACK_PAWN)&&(!pieceTypeOf(piece_on(Square(dsq+8)))))
				defenders[sq]=1;
		}
		if(r<RANK_7)
		{
			// find out if a black pawn can move by one square
			sq = Square(dsq+8);
			if(piece_on(sq)==BLACK_PAWN)
				defenders[sq]=1;
		}
	}
	else
	{   // white's turn to move
		r = ranksOfSquares[dsq];
		if(r==RANK_4)
		{   // find out if a white pawn can move by two squares
			sq = Square(dsq-16);
			if ((piece_on(sq)==WHITE_PAWN)&&(!pieceTypeOf(piece_on(Square(dsq-8)))))
				defenders[sq]=1;
		}
		if(r>RANK_2)
		{
			// find out if a white pawn can move by one square
			sq = Square(dsq-8);
			if(piece_on(sq)==WHITE_PAWN)
				defenders[sq]=1;
		}
	}

	/// Can we move a KNIGHT to Square dsq?..
	p_our = Piece(WHITE_KNIGHT + sideToMove);
	p_their = Piece(WHITE_KNIGHT + ~sideToMove);
	dir=0;
    while ((dir<MAX_DIRECTIONS)&&(protoMoves[p_their][dsq][dir][0]!=NO))
	{
		sq = protoMoves[p_their][dsq][dir][0];
		if(piece_on(sq)==p_our) // found a knight defender
		{
			// Enter the defender info into the defenders bitboard:
			defenders[sq]=1;
		}
		dir++;
	}

	/// Can we move our QUEEN or ROOK to Square dsq
	/// from a horizontal or vertical direction?
	// if we're playing white, imagine a black rook in Square dsq;
	// otherwise imagine a white rook in the square.
	// Now explore the rook's capture directions to find out if
	// a queen or rook of our color is there:
	p_our = Piece(WHITE_ROOK + sideToMove);
	p_their = Piece(WHITE_ROOK + ~sideToMove);
	p_our_too = Piece(WHITE_QUEEN + sideToMove);
	dir=0;
    while (protoMoves[p_their][dsq][dir][0]!=NO) // with rooks, we won't exhaust directions
	{
		opt=0;
		do
		{
			sq = protoMoves[p_their][dsq][dir][opt];
			p = piece_on(sq);
			if (p > EN_PASSANT) // if the square is NOT (empty OR en_passant)
			{
				if ((p==p_our)||(p==p_our_too)) // found our rook or queen
				{
					// Enter the defender info into the defenders bitboard:
					defenders[sq]=1;
				}
				break;
			}
			opt++;
		}
		while (protoMoves[p_their][dsq][dir][opt]!=NO);
		dir++;
	}

	/// Can we move our QUEEN or BISHOP to Square dsq from a diagonal direction?
	// If we're playing white, imagine a black bishop in Square dsq;
	// otherwise imagine a white bishop in the square.
	// Now explore the bishop's capture directions to find out if
	// a queen or bishop of our color is there:
	p_our = Piece(WHITE_BISHOP + sideToMove);
	p_their = Piece(WHITE_BISHOP + ~sideToMove);
	p_our_too = Piece(WHITE_QUEEN + sideToMove);
	dir=0;
    while (protoMoves[p_their][dsq][dir][0]!=NO) // with bishops, we won't exhaust directions
	{
		opt=0;
		do
		{
			sq = protoMoves[p_their][dsq][dir][opt];
			p = piece_on(sq);
			if (p > EN_PASSANT) // if the square is NOT (empty OR en_passant)
			{
				if ((p==p_our)||(p==p_our_too)) // found our bishop or queen
				{
					// Enter the defender info into the defenders bitboard:
					defenders[sq]=1;
				}
				break;
			}
			opt++;
		}
		while (protoMoves[p_their][dsq][dir][opt]!=NO);
		dir++;
	}

	return defenders;
}

/// Position::move_gives_check() tests whether a move gives a check
bool Position::move_gives_check(Move m, const CheckInfo& ci) const
{
	Piece p;
	// D.A.G., 04/19/2012: Comment out asserts inherited from Stockfish 2.2.2
	/*
	assert(is_ok(m));
	assert(ci.dcCandidates == discovered_check_candidates());
	assert(color_of(piece_moved(m)) == sideToMove);
	*/

	Square from = from_sq(m);
	Square to = to_sq(m);
	Square enemy_ksq = king_square(~sideToMove);
	PieceType pt = pieceTypeOf(piece_on(from));

	// Discovery check? Verify direction of the candidate piece move.
	// If a candidate knight moves, it's a discovered check for sure.
	if (ci.dcCandidates[from] && ((pt==KNIGHT)||!same_line(from, to, enemy_ksq)))
		return true;

	// Account for piece type change for promotions:
	if (is_promotion(m))
		pt = m.promotionPieceType;
	// Direct check?
	if (ci.checkSq[pt][to])
		return true;

	// Can we skip the ugly special cases (en passant, castle, promotion)?
	if (!is_special(m))
		return false;

	// EN_PASSANT square was treated as a piece and added to occupied squares,
	// but it's transparent! So...
	Bitboard occupied = occupied_squares()^SquareBB[ep_square()];
	//print_bitboard(occupied,"Occupied:"); // for debugging

	// Promotion with check? The promotion piece can give check "backward"
	// through the square previously occupied by the promoted pawn!
	// This matters only for newly added sliding pieces: queens, rooks, and bishops.
	if (is_promotion(m)&&(pieceCategoryOf(make_piece(sideToMove,pt))==SLIDING_PIECE))
	{
		if ((!file_difference(from,to))&&(!file_difference(from,enemy_ksq)))
		{   // from, to, and enemy_ksq are in the same file
			if (pt!=BISHOP)
			{
				// trace back along the vertical to make sure it's clear
				// from the pawn promoted to queen or rook all the way to the enemy king
				if ((BetweenBB[from][enemy_ksq]&occupied).none())
					return true;
			}
		}
		else if ((file_distance(from,enemy_ksq)==rank_distance(from,enemy_ksq))&&
			(file_distance(to,enemy_ksq)==rank_distance(to, enemy_ksq))&&
			(file_difference(from,enemy_ksq)*rank_difference(from,enemy_ksq)*
			file_difference(to,enemy_ksq)*rank_difference(to,enemy_ksq)>0))
		{   // from, to, and enemy_ksq are in the same diagonal
			if (pt!=ROOK)
			{
				// trace back along the diagonal to make sure it's clear
				// from the pawn promoted to queen or bishop all the way to the enemy king
				if ((BetweenBB[from][enemy_ksq]&occupied).none())
					return true;
			}
		}
	}

	// En passant capture with check? We have already handled the case
	// of direct checks and ordinary discovered check, the only case we
	// need to handle is the unusual case of a discovered check through
	// the captured pawn.
	if (is_enpassant(m))
	{
		// Compute capsq, the square where the pawn captured en passant was:
		Square capsq = make_square(file_of(to), rank_of(from));
		// D.A.G., 04/19/2012: Commented out the solution from Stockfish 2.2.2.
		/*
		occupied ^= from;
		occupied ^= capsq;
		occupied |= to;
		return  (rook_attacks_bb(ksq, b) & pieces(ROOK, QUEEN, us))
			||(bishop_attacks_bb(ksq, b) & pieces(BISHOP, QUEEN, us));
			*/
		if (sideToMove) // black's turn to move
		{
			if(ranksOfSquares[enemy_ksq]==RANK_4)
			{   // Find out if a rook or queen gives check along RANK_4
				if ((Rank4BB&(pieces(ROOK,QUEEN,BLACK))).any())
				{   // test if our rook or queen opens up
					if (filesOfSquares[enemy_ksq] < filesOfSquares[from])
					{
						for(Square i = Square(enemy_ksq+1);filesOfSquares[i]<=FILE_H; i = Square(i+1))
						{
							p = piece_on(i);
							if((p==BLACK_ROOK)||(p==BLACK_QUEEN))
								return true;
							else if (((colorOf(p)==BLACK)&&(i!=from))||
								((colorOf(p)==WHITE)&&(i!=capsq)))
								break;
						}
						return false;
					}
					else // filesOfSquares[enemy_ksq] > filesOfSquares[sq]
					{
						for(Square i = Square(enemy_ksq-1);filesOfSquares[i]>=FILE_A; i = Square(i-1))
						{
							p = piece_on(i);
							if((p==BLACK_ROOK)||(p==BLACK_QUEEN))
								return true;
							else if (((colorOf(p)==BLACK)&&(i!=from))||
								((colorOf(p)==WHITE)&&(i!=capsq)))
								break;
						}
						return false;
					}
				}
				else // the enemy king is in RANK_4, but we have no rook or queen in RANK_4
					return false;
			}
			else if ((file_distance(capsq,enemy_ksq)==rank_distance(capsq,enemy_ksq))&&
				(BetweenBB[capsq][enemy_ksq]&occupied).none())
			{   // Find out if a bishop or queen gives check along a diagonal through capsq
				int delta = (capsq - enemy_ksq) / square_distance(enemy_ksq, capsq);

				for (Square i = capsq;; i = Square(i+delta))
				{
					File f = filesOfSquares[i];
					if (i==capsq)
					{
						if ((f==FILE_A)||(f==FILE_H))
							break;
					}
					else
					{
						Rank r = ranksOfSquares[i];
						p = piece_on(i);
						if((p==BLACK_BISHOP)||(p==BLACK_QUEEN))
							return true;
						else if ((colorOf(p)!=NO_COLOR)||(r==RANK_1)||(r==RANK_8)||(f==FILE_A)||(f==FILE_H))
							break;
					}
				}
				return false;
			}
		}
		else // white's turn to move
		{
			if(ranksOfSquares[enemy_ksq]==RANK_5)
			{   // Find out if a rook or queen gives check along RANK_5
				if ((Rank5BB&(pieces(ROOK,QUEEN,WHITE))).any())
				{   // test if our rook or queen opens up
					if (filesOfSquares[enemy_ksq] < filesOfSquares[from])
					{
						for(Square i = Square(enemy_ksq+1);filesOfSquares[i]<=FILE_H; i = Square(i+1))
						{
							p = piece_on(i);
							if((p==WHITE_ROOK)||(p==WHITE_QUEEN))
								return true;
							else if (((colorOf(p)==WHITE)&&(i!=from))||
								((colorOf(p)==BLACK)&&(i!=capsq)))
								break;
						}
						return false;
					}
					else // filesOfSquares[enemy_ksq] > filesOfSquares[sq]
					{
						for(Square i = Square(enemy_ksq-1);filesOfSquares[i]>=FILE_A; i = Square(i-1))
						{
							p = piece_on(i);
							if((p==WHITE_ROOK)||(p==WHITE_QUEEN))
								return true;
							else if (((colorOf(p)==WHITE)&&(i!=from))||
								((colorOf(p)==BLACK)&&(i!=capsq)))
								break;
						}
						return false;
					}
				}
				else // the enemy king is in RANK_5, but we have no rook or queen in RANK_5
					return false;
			}
			else if (file_distance(capsq,enemy_ksq)==rank_distance(capsq,enemy_ksq)&&
				(BetweenBB[capsq][enemy_ksq]&occupied).none())
			{   // Find out if a bishop or queen gives check along a diagonal through capsq
				int delta = (capsq - enemy_ksq) / square_distance(enemy_ksq, capsq);

				for (Square i = capsq;; i = Square(i+delta))
				{
					File f = filesOfSquares[i];
					if (i==capsq)
					{
						if ((f==FILE_A)||(f==FILE_H))
							break;
					}
					else
					{
						Rank r = ranksOfSquares[i];
						p = piece_on(i);
						if((p==WHITE_BISHOP)||(p==WHITE_QUEEN))
							return true;
						else if ((colorOf(p)!=NO_COLOR)||(r==RANK_1)||(r==RANK_8)||(f==FILE_A)||(f==FILE_H))
							break;
					}
				}
				return false;
			}
		}
	}

	// Castling with check ?
	if (is_castle(m))
	{
		if (sideToMove) // black side to move
		{
			if(m.moveFlags[M_CASTLE_SHORT])
				return ((filesOfSquares[enemy_ksq]==FILE_F)&&(BetweenBB[F8][enemy_ksq]&occupied).none())||
				((ranksOfSquares[enemy_ksq]==RANK_8)&&(BetweenBB[E8][enemy_ksq]&occupied).none());
			else // M_CASTLE_LONG
				return ((filesOfSquares[enemy_ksq]==FILE_D)&&(BetweenBB[D8][enemy_ksq]&occupied).none())||
				((ranksOfSquares[enemy_ksq]==RANK_8)&&(BetweenBB[E8][enemy_ksq]&occupied).none());
		}
		else // white side to move
		{
			if(m.moveFlags[M_CASTLE_SHORT])
				return ((filesOfSquares[enemy_ksq]==FILE_F)&&(BetweenBB[F1][enemy_ksq]&occupied).none())||
				((ranksOfSquares[enemy_ksq]==RANK_1)&&(BetweenBB[E1][enemy_ksq]&occupied).none());
			else // M_CASTLE_LONG
				return ((filesOfSquares[enemy_ksq]==FILE_D)&&(BetweenBB[D1][enemy_ksq]&occupied).none())||
				((ranksOfSquares[enemy_ksq]==RANK_1)&&(BetweenBB[E1][enemy_ksq]&occupied).none());
		}
	}
	return false;
}
