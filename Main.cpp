#include "RingBufTest.h"


int main()
{
	TestRingBuffer Test;

	Test.ManualRealTimeTest();
	//Test.MultipleProducersSingleConsumerTest();
	//Test.SingleProducerMultipleConsumerTest();
	//Test.MultipleProducersMultipleConsumersTest();
	return 0;
}
