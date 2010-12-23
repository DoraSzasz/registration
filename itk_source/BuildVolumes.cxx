// YAML config reader
#include "yaml.h"

// ITK includes
#include "itkImage.h"
#include "itkImageRegistrationMethod.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkImage.h"
#include "itkResampleImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkImageMaskSpatialObject.h"

// File IO
#include "itkRegularExpressionSeriesFileNames.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkTransformFileWriter.h"
#include "itkTransformFactory.h"

// my files
#include "StdOutIterationUpdate.hpp"
#include "FileIterationUpdate.hpp"
#include "MultiResRegistrationCommand.hpp"
#include "Stack.hpp"
#include "Framework2DRat.hpp"
#include "helper_functions.hpp"
#include "TransformInitializers.hpp"
#include "itkSimilarity2DTransform.h"
#include "Dirs.hpp"

// TEMP
#include "itkTransformFileWriter.h"
// TEMP

void checkUsage(int argc, char const *argv[]);

int main(int argc, char const *argv[]) {
	// Verify the number of parameters in the command line and apply meaningful names to each
	checkUsage(argc, argv);
  string LoResDir(argv[1]), HiResDir(argv[2]), outputDir(argv[3]);
	
	// read registration parameters
  YAML::Node registrationParameters;
  readRegistrationParameters(registrationParameters, Dirs::ParamsFile());
  cout << Dirs::ParamsFile() << endl;
	
	// initialise stack objects
  Stack::VolumeType::SpacingType LoResSpacings, HiResSpacings;
	for(unsigned int i=0; i<3; i++) {
    registrationParameters["LoResSpacings"][i] >> LoResSpacings[i];
    registrationParameters["HiResSpacings"][i] >> HiResSpacings[i];
  }
  
  Stack::SliceType::SizeType LoResSize, LoResOffset;
  for(unsigned int i=0; i<2; i++) {
    registrationParameters["LoResSize"][i] >> LoResSize[i];
    registrationParameters["LoResOffset"][i] >> LoResOffset[i];
  }
  
  // Stack LoResStack( getFileNames(LoResDir, Dirs::SliceFile), LoResSpacings , LoResSize, LoResOffset);
  // Stack HiResStack( getFileNames(HiResDir, Dirs::SliceFile), HiResSpacings );
  // TEMP
  HiResSpacings[0] = 1;
  HiResSpacings[1] = 1;
  HiResSpacings[2] = 1;
  LoResSpacings[0] = 1;
  LoResSpacings[1] = 1;
  LoResSpacings[2] = 1;
  
  vector< string > brain, rotatedBrain;
  brain.push_back(LoResDir + "10000.png");
  rotatedBrain.push_back(HiResDir + "10000.png");
  Stack LoResStack( brain, LoResSpacings );
  Stack HiResStack( rotatedBrain, HiResSpacings );
  
  // TEMP
  
  
  if (LoResStack.GetSize() != HiResStack.GetSize()) { cerr << "LoRes and HiRes stacks are different sizes!" << endl;}
  
  // initialize stacks' transforms so that 2D images line up at their centres.
  InitializeStackTransforms::ToCommonCentre( LoResStack );
  InitializeStackTransforms::ToCommonCentre( HiResStack );
  
  LoResStack.updateVolumes();
  HiResStack.updateVolumes();
  
  Framework2DRat framework2DRat(&LoResStack, &HiResStack, registrationParameters);
  
  // Set optimizer scales for CenteredRigid2DTransform
  double translationScale;
  registrationParameters["optimizer"]["translationScale"] >> translationScale;
	itk::Array< double > rigidOptimizerScales( 5 );
  rigidOptimizerScales[0] = 1.0;
  rigidOptimizerScales[1] = translationScale;
  rigidOptimizerScales[2] = translationScale;
  rigidOptimizerScales[3] = translationScale;
  rigidOptimizerScales[4] = translationScale;
  framework2DRat.GetOptimizer()->SetScales( rigidOptimizerScales );
  
	// perform centered rigid 2D registration
  framework2DRat.StartRegistration();
  
  HiResStack.updateVolumes();
  
  
  // TEMP
  // write transform
  itk::TransformFileWriter::Pointer writer;
  writer = itk::TransformFileWriter::New();
  
  writer->SetFileName( "Transforms.meta" );
  writer->AddTransform( HiResStack.GetTransform(0) );
  
  // Software Guide : EndCodeSnippet
  try
    {
    // Software Guide : BeginCodeSnippet
    writer->Update();
    // Software Guide : EndCodeSnippet
    }
  catch( itk::ExceptionObject & excp )
    {
    std::cerr << "Error while saving the transforms" << std::endl;
    std::cerr << excp << std::endl;
    return 0;
    }
  
  // TEMP
  
  //   InitializeStackTransforms::FromCurrentTransforms< itk::Similarity2DTransform< double > >( HiResStack );
  //   
  //   // Set optimizer scales for Similarity2DTransform
  // Framework2DRat::OptimizerType::ScalesType similarityOptimizerScales( 4 );
  //   similarityOptimizerScales[0] = 1.0;
  //   similarityOptimizerScales[1] = 1.0;
  //   similarityOptimizerScales[2] = translationScale;
  //   similarityOptimizerScales[3] = translationScale;
  //   framework2DRat.GetOptimizer()->SetScales( similarityOptimizerScales );
  //   
  //   framework2DRat.StartRegistration( outputDir + "/output2.txt" );
  //   
  //   // Update LoRes as the masks might have shrunk, HiRes as the transforms have changed
  //   LoResStack.updateVolumes();
  //   HiResStack.updateVolumes();
  
  // Write final transform to file
  // writeData< itk::TransformFileWriter, Framework3D::TransformType3D >( framework3D.transform3D, outputDir + "/finalParameters3D.transform" );
  
	// perform non-rigid registration
	// check out itkMultiResolutionPDEDeformableRegistration
	// update HiRes slices
  // HiResStack.updateVolumes();
	// write volume and mask to disk
	writeImage< Stack::VolumeType >( LoResStack.GetVolume(), outputDir + "/LoResStack.mha" );
  writeImage< Stack::MaskVolumeType >( LoResStack.Get3DMask()->GetImage(), outputDir + "/LoResMask.mha" );
	writeImage< Stack::VolumeType >( HiResStack.GetVolume(), outputDir + "/HiResStack.mha" );
  writeImage< Stack::MaskVolumeType >( HiResStack.Get3DMask()->GetImage(), outputDir + "/HiResMask.mha" );
	
  return EXIT_SUCCESS;
}

void checkUsage(int argc, char const *argv[]) {
  if( argc != 4 )
  {
    cerr << "\nUsage: " << endl;
    cerr << argv[0] << " LoResDir HiResDir outputDir\n\n";
    exit(EXIT_FAILURE);
  }
}
