#include "OCR_App.h"

int main() 
{
	OCR_App ocr;

	while (ocr.isOpen())
	{
		ocr.update();
	}

	return 0;
}