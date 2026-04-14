#include "cassert"

const unsigned int SEQ_CAPACITY = 24;

class SequenceBuffer{

    private:
        float sequence[SEQ_CAPACITY];
        unsigned int head = 0;
        unsigned int count = 0;
        

    public:
        bool isReady();
        void push(float value);
        float operator[](unsigned int i) const;
        

};