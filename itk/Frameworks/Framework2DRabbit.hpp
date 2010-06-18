// This object constructs and encapsulates the 2D registration framework.

#ifndef FRAMEWORK2DRABBIT_HPP_
#define FRAMEWORK2DRABBIT_HPP_

// my files
#include "Stack.hpp"
#include "MRI.hpp"
#include "Framework2DBase.hpp"


class Framework2DRabbit : public Framework2DBase {
public:
	
	Stack *stack;
	MRI *mriVolume;  
	
	Framework2DRabbit(Stack *inputStack, MRI *inputMriVolume, YAML::Node& parameters):
	Framework2DBase(parameters),
	stack(inputStack),
	mriVolume(inputMriVolume) {
	  
	}
	
	void StartRegistration(const string& outputFileName) {
		observerOutput.open( outputFileName.c_str() );
    
    unsigned int number_of_slices = stack->originalImages.size();
    
    for(unsigned int slice_number=0; slice_number < number_of_slices; slice_number++) {
  	  registration2D->SetFixedImage( mriVolume->GetResampledSlice(slice_number) );
  		registration2D->SetMovingImage( stack->originalImages[slice_number] );
	    
  	  // TEST TO SEE IF THIS MAKES ANY DIFFERENCE
  	  registration2D->SetFixedImageRegion( mriVolume->GetResampledSlice(slice_number)->GetLargestPossibleRegion() );
  	  // TEST TO SEE IF THIS MAKES ANY DIFFERENCE
	    
  		metric2D->SetFixedImageMask( mriVolume->GetMask2D(slice_number) );
  		metric2D->SetMovingImageMask( stack->GetMask2D(slice_number) );
  		
  		registration2D->SetTransform( stack->GetTransform(slice_number) );
  		registration2D->SetInitialTransformParameters( stack->GetTransform(slice_number)->GetParameters() );
  		
  	  try
  	    {
  	    registration2D->StartRegistration();
  	    cout << "Optimizer stop condition: "
  	         << registration2D->GetOptimizer()->GetStopConditionDescription() << endl << endl;
  	    }
  	  catch( itk::ExceptionObject & err )
  	    {
  	    std::cerr << "ExceptionObject caught !" << std::endl;
  	    std::cerr << err << std::endl;
  	    exit(EXIT_FAILURE);
  	    }
    }
    
	  observerOutput.close();
	}
	
protected:
};
#endif
