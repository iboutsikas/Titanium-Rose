UINT DispatchSize(UINT ThreadGroupSize, UINT NumElements)
{
    return (NumElements + (ThreadGroupSize - 1)) / ThreadGroupSize;
}