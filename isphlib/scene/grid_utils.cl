R"(

#define KERNEL_SUPPORT_SQ (KERNEL_SUPPORT*KERNEL_SUPPORT)

#if DIM == 3


int4 CellPos(vector pos)
{
	//vector gridPos = floor((pos - gridStart) * cellSizeInv);
    //return convert_int4(gridPos);
	return (int4)(
	(int)floor((pos.x-GRID_START.x)*CELL_SIZE_INV),
	(int)floor((pos.y-GRID_START.y)*CELL_SIZE_INV),
	(int)floor((pos.z-GRID_START.z)*CELL_SIZE_INV),
	0);
}

int CellHash(int4 cell)
{
	if(cell.x >= CELL_COUNT.x || cell.y >= CELL_COUNT.y || cell.z >= CELL_COUNT.z)
		return -1;
	if(cell.x < 0 || cell.y < 0 || cell.z < 0)
		return -1;
	return cell.x + (cell.y * CELL_COUNT.x) + (cell.z * CELL_COUNT.x * CELL_COUNT.y);
}

#define ForEachSetup(POS) \
	int4 _cellI = CellPos(POS); \
	int4 _loopStart = max(_cellI-(int4)1, (int4)0); \
	int4 _loopEnd = min(_cellI+(int4)1, CELL_COUNT_1); \
	int4 _cellJ;

#define ForEachNeighbor(HASHES,CELLS_START,POSITIONS,POS_I) \
	for(_cellJ.x=_loopStart.x; _cellJ.x<=_loopEnd.x; _cellJ.x++) \
	for(_cellJ.y=_loopStart.y; _cellJ.y<=_loopEnd.y; _cellJ.y++) \
	for(_cellJ.z=_loopStart.z; _cellJ.z<=_loopEnd.z; _cellJ.z++){ \
		int _hash = CellHash(_cellJ); \
		uint _j = CELLS_START[_hash]; \
		if(_j == UINT_MAX) continue; \
		for(int2 particleJ=HASHES[_j]; _hash==particleJ.x; particleJ=HASHES[++_j]){ \
			int j = particleJ.y; if(j==i) continue; \
			vector posDif = POS_I - POSITIONS[j]; \
			scalar QSq = dot(posDif, posDif) * SMOOTHING_LENGTH_INV_SQ; \
			if(QSq >= KERNEL_SUPPORT_SQ) continue;

#else


int2 CellPos(vector pos)
{
	//return convert_int2(floor(pos - gridStart) * cellSizeInv);
	return (int2)((int)floor((pos.x-GRID_START.x)*CELL_SIZE_INV), (int)floor((pos.y-GRID_START.y)*CELL_SIZE_INV));
}

int CellHash(int2 cell)
{
	//cell = min(cell, cellCount-(uint2)1u);
   //if(cell.x >= CELL_COUNT.x || cell.y >= CELL_COUNT.y)
   //	return -1;
   //if(cell.x < 0 || cell.y < 0)
   //	return -1;
	return cell.y * CELL_COUNT.x + cell.x;
}

#define ForEachSetup(POS) \
   int2 _loopStart = CellPos(POS - KERNEL_SUPPORT_RADIUSES); \
   int2 _loopEnd   = CellPos(POS + KERNEL_SUPPORT_RADIUSES); \
	int2 _cellJ;

#define ForEachNeighbor(HASHES,CELLS_START,POSITIONS,POS_I) \
	for(_cellJ.y=_loopStart.y; _cellJ.y<=_loopEnd.y; _cellJ.y++) \
	for(_cellJ.x=_loopStart.x; _cellJ.x<=_loopEnd.x; _cellJ.x++){ \
		int _hash = CellHash(_cellJ); \
		uint _j = CELLS_START[_hash]; \
      if(_j != UINT_MAX){ \
         for(int2 particleJ=HASHES[_j]; _hash==particleJ.x; particleJ=HASHES[++_j]){ \
            int j = particleJ.y; if(j!=i) { \
               vector posDif = POS_I - POSITIONS[j]; \
               scalar QSq = dot(posDif, posDif) * SMOOTHING_LENGTH_INV_SQ; \
               if(QSq < KERNEL_SUPPORT_SQ) {

#endif


#define ForEachEnd }}}}}

)" /* end OpenCL code */
