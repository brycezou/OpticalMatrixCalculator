#include <iostream>
#include "fann.h"

using namespace std;


class ittFann
{
private:
	unsigned int num_input;
	unsigned int num_output;
	unsigned int num_layers;
	unsigned int num_hidden;
public:
	struct fann *m_ann;
	ittFann();
	ittFann(unsigned int nInput, unsigned int nOutput, unsigned int nLayers, unsigned int nHidden);
	void fannInitial(fann_activationfunc_enum fun_hidden, fann_activationfunc_enum fun_output);
	void fannTrain(char *filename, unsigned int max_epochs, unsigned int epochs_between_reports, float desired_error);
	void fannSaveModel(char *filename);
	void fannRelease();
	bool fannCreateFromFile(char *filename, bool bShow = true);
	fann_type* fannPredict(fann_type *input, int nInput);
};
