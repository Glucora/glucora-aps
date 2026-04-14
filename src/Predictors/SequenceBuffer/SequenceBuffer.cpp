#include "SequenceBuffer.hpp"


bool SequenceBuffer::isReady(){
    return count == SEQ_CAPACITY;
}

void SequenceBuffer::push(float value)
{
    sequence[head] = value;
    head = (head +1 ) % SEQ_CAPACITY;
    if(count < SEQ_CAPACITY) ++count;
}

float SequenceBuffer::operator[](unsigned int i) const
{
    assert(i < count);
    unsigned int oldestIndex = (this->head + SEQ_CAPACITY - count) % SEQ_CAPACITY;
    return sequence[(oldestIndex+i) % SEQ_CAPACITY];
}
