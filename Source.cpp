#include <string>

// ticket #1
bool isEven(int value) { return value & 1; }

// ticket #2-1 
// good - fast
// good - save
// bad  - uncompact
using UINT = unsigned int;
using PVOID = void*;

template <typename Type, size_t size>
class FastUncompact
{
private:
	UINT mPosition = size - 1;
	Type pBuffer[size] = {};
public:
	FastUncompact& Pop(Type& type)
	{
		pBuffer[mPosition = mPosition + 1 == size ? 0 : mPosition + 1] = type;
		return *this;
	};

	FastUncompact& Push(Type& type)
	{
		type = pBuffer[mPosition ? (mPosition -= 1) + 1 : (mPosition = size - 1) - size + 1];
		return *this;
	};
};

// ticket #2-2
// bad  - slow
// bad  - unsave
// good - compact
template <size_t size>
class SlowCompact
{
private:
	UINT  mPosition = size - 1;
	PVOID pBuffer[size] = {};
public:
	template <typename Type>
	SlowCompact& Pop(Type& type)
	{
		mPosition = mPosition + 1 == size ? 0 : mPosition + 1;
		if (pBuffer[mPosition])
			delete pBuffer[mPosition];

		pBuffer[mPosition] = new Type;
		memcpy(pBuffer[mPosition], &type, sizeof(type));

		return *this;
	}
	template <typename Type>
	SlowCompact& Push(Type& type)
	{
		if (pBuffer[mPosition])
		{
			memcpy(&type, pBuffer[mPosition], sizeof(type));
			delete pBuffer[mPosition];
			pBuffer[mPosition] = nullptr;
		}

		mPosition = mPosition - 1 == -1 ? mPosition = size - 1 : mPosition -= 1;
		return *this;
	}
};

struct STest
{
	UINT x;
	UINT y;
	UINT z;
};

// ticket #3-1 TrivialQuicksort
UINT quickSortR(UINT N, UINT* a)
{
	UINT time;
	__asm
	{
		push edx
		rdtsc
		pop  edx
		mov time, eax
	}

	UINT i = 0, j = N - 1, temp, p = a[N >> 1] - 1;

	do {
		while (a[i] < p) i++;
		while (a[j] > p) j--;

		if (i < j) {
			temp = a[i]; a[i] = a[j]; a[j] = temp;
			i++; j--;
		}
	} while (i < j);

	if (i == j) j++;

	if (j > 2) quickSortR(j, a);
	if ((N - j) > 2) quickSortR(N - i, a + i);

	__asm
	{
		rdtsc
		sub eax, time
		mov time, eax
	}
	return time;
}

// ticket #3-2 UltraFastQuicksort
// 
__declspec(naked) UINT __fastcall UltraSort(const UINT size, const UINT* massif)
{
	#define xmmword 0x10
	__asm
	{
		push edx
		rdtsc
		pop  edx
		push eax

		movdqa xmm1, [edx + xmmword * 0]
		movdqa xmm2, [edx + xmmword * 1]
		movdqa xmm0, xmm2
		pmaxud xmm2, xmm1
		pmaxud xmm1, xmm0

		movdqa xmm4, [edx + xmmword * 2]
		movdqa xmm5, [edx + xmmword * 3]
		movdqa xmm3, xmm5
		pmaxud xmm5, xmm4
		pminud xmm4, xmm3

		movdqa xmm0, xmm1
		pminud xmm0, xmm4

		movdqa xmm3, xmm2
		pmaxud xmm3, xmm5

		pminud xmm2, xmm5
		pmaxud xmm1, xmm4
		movdqa xmm4, xmm1
		pminud xmm1, xmm2
		pmaxud xmm2, xmm4

		movdqa xmm4, xmm0
		punpckhdq xmm4, xmm1
		movdqa xmm5, xmm2
		punpckldq xmm5, xmm3

		punpckldq  xmm0, xmm1
		movdqa     xmm1, xmm0
		punpcklqdq xmm0, xmm5
		punpckhqdq xmm1, xmm5

		punpckhdq  xmm2, xmm3
		movdqa     xmm3, xmm4
		punpckhqdq xmm3, xmm2
		punpcklqdq xmm4, xmm2

		mov ecx, 4
		m00:
			movdqa xmm2, xmm1
			pmaxud xmm1, xmm0
			pminud xmm0, xmm2
			pshufd xmm1, xmm1, 10010011b

			movdqa xmm5, xmm4
			pmaxud xmm4, xmm3
			pminud xmm3, xmm5
			pshufd xmm4, xmm4, 10010011b
			dec ecx
		jnz m00

			mov ecx, 8
		m01:
			movdqa xmm2, xmm3
			movdqa xmm5, xmm4

			pmaxud xmm3, xmm0
			pmaxud xmm4, xmm1

			pminud xmm0, xmm2
			pminud xmm1, xmm5

			pshufd xmm3, xmm3, 10010011b
			pshufd xmm4, xmm4, 10010011b
			movd    eax, xmm3
			movss  xmm3, xmm4
			pinsrd xmm4, eax, 0
			dec ecx
		jnz m01

		movdqa [edx + xmmword * 0], xmm0
		movdqa [edx + xmmword * 1], xmm1
		movdqa [edx + xmmword * 2], xmm2
		movdqa [edx + xmmword * 3], xmm3
		
		rdtsc
		pop edx
		sub eax, edx
		ret
	}
}

constexpr UINT maxMass = 0x10;
UINT massif[maxMass];

int main()
{
	STest test[] = { { 1,1,1 },{ 2,2,2 },{ 3,3,3 } };

	FastUncompact<STest, 2> FastUncompactCircle;
	FastUncompactCircle.Pop(test[0]).Pop(test[1]).Pop(test[2]).Push(test[0]);

	SlowCompact<2> SlowCompactCircle;
	SlowCompactCircle.Pop(test[0]).Pop(test[1]).Pop(test[2]).Push(test[0]);

	for (UINT index = 0; index < maxMass; index++)
		massif[index] = maxMass - index;

	UINT time1 = UltraSort(maxMass, massif);

	for (UINT index = 0; index < maxMass; index++)

		massif[index] = index;
	UINT time2 = quickSortR(maxMass, massif);
}