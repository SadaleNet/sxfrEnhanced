char bufferForConvertion[32];
char* ftoa(float value){
	sprintf(bufferForConvertion, "%.3f", value);
	return bufferForConvertion;
}

char* itoa(int value){
	sprintf(bufferForConvertion, "%i", value);
	return bufferForConvertion;
}
