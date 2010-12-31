#ifndef _STACK_CXX_
#define _STACK_CXX_

#include <sys/stat.h> // for fileExists
#include "Stack.hpp"


Stack::Stack(const vector< string >& inputFileNames, const VolumeType::SpacingType& inputSpacings):
fileNames(inputFileNames),
spacings(inputSpacings) {
  readImages();
  initializeVectors();
	// scale slices and initialise volume and mask
  offset.Fill(0);
  resamplerSize.Fill(0);
  for(unsigned int i=0; i<2; i++) originalSpacings[i] = spacings[i];
  scaleOriginalSlices();
  buildOriginalMaskSlices();
  calculateMaxSize();
  setResamplerSizeToMaxSize();
  initializeFilters();
}

// constructor to specify size and offset
Stack::Stack(const vector< string >& inputFileNames, const VolumeType::SpacingType& inputSpacings, const SliceType::SizeType& inputSize, const SliceType::SizeType& inputOffset):
fileNames(inputFileNames),
resamplerSize(inputSize),
offset(inputOffset),
spacings(inputSpacings) {
  readImages();
  initializeVectors();
	// scale slices and initialise volume and mask
	for(unsigned int i=0; i<2; i++) originalSpacings[i] = spacings[i];
  scaleOriginalSlices();
  buildOriginalMaskSlices();
  calculateMaxSize();
  initializeFilters();
}
	
void Stack::readImages() {
  originalImages.clear();
  ReaderType::Pointer reader;
	
	for(unsigned int i=0; i<fileNames.size(); i++) {
	  if( fileExists(fileNames[i]) ) {
			reader = ReaderType::New();
			reader->SetFileName( fileNames[i] );
			reader->Update();
			originalImages.push_back( reader->GetOutput() );
			originalImages.back()->DisconnectPipeline();
	  }
	  else
	  {
	    // create a new image of zero size
      originalImages.push_back( SliceType::New() );
	  }
	}
	
}
  
void Stack::initializeVectors() {
	// initialise various data members once the number of images is available
	numberOfTimesTooBig = vector< unsigned int >( GetSize(), 0 );
  for(unsigned int slice_number = 0; slice_number < GetSize(); slice_number++) {
    slices.push_back( SliceType::New() );
    original2DMasks.push_back( MaskType2D::New() );
    resampled2DMasks.push_back( MaskType2D::New() );
  }
}
  
void Stack::scaleOriginalSlices() {
  // rescale original images
	for(unsigned int slice_number=0; slice_number<originalImages.size(); slice_number++) {
	  xyScaler = XYScaleType::New();
		xyScaler->ChangeSpacingOn();
		xyScaler->SetOutputSpacing( originalSpacings );
    xyScaler->SetInput( originalImages[slice_number] );
    xyScaler->Update();
    originalImages[slice_number] = xyScaler->GetOutput();
    originalImages[slice_number]->DisconnectPipeline();
	}
}
	
void Stack::buildOriginalMaskSlices() {
	// build a vector of mask slices
	for(unsigned int slice_number=0; slice_number<originalImages.size(); slice_number++) {
		// make new maskSlice and make it all white
		MaskSliceType::RegionType region;
		region.SetSize( originalImages[slice_number]->GetLargestPossibleRegion().GetSize() );
	  
		MaskSliceType::Pointer maskSlice = MaskSliceType::New();
		maskSlice->SetRegions( region );
		maskSlice->CopyInformation( originalImages[slice_number] );
	  maskSlice->Allocate();
		maskSlice->FillBuffer( 255 );
	  
    // assign mask slices to masks
    original2DMasks[slice_number]->SetImage( maskSlice );
    
  }
}
	
void Stack::calculateMaxSize() {
	SliceType::SizeType size;
	maxSize.Fill(0);
	unsigned int dimension = size.GetSizeDimension();
  
	for(unsigned int slice_number=0; slice_number<originalImages.size(); slice_number++) {
		size = originalImages[slice_number]->GetLargestPossibleRegion().GetSize();

		for(unsigned int i=0; i<dimension; i++)
		{
			if(maxSize[i] < size[i]) { maxSize[i] = size[i]; }
		}
	}
}
	
void Stack::setResamplerSizeToMaxSize() {
  for(unsigned int i=0; i<2; i++) {
    resamplerSize[i] = maxSize[i];
  }
}

void Stack::initializeFilters() {
	// resamplers
	linearInterpolator = LinearInterpolatorType::New();
	nearestNeighborInterpolator = NearestNeighborInterpolatorType::New();
	resampler = ResamplerType::New();
	resampler->SetInterpolator( linearInterpolator );
	resampler->SetSize( resamplerSize );
  // resampler->SetOutputOrigin( toSomeSensibleValue );
  // resampler->SetOutputDirection( originalImages[slice_number]->GetDirection() );
	resampler->SetOutputSpacing( spacings2D() );
	maskResampler = MaskResamplerType::New();
	maskResampler->SetInterpolator( nearestNeighborInterpolator );
	maskResampler->SetSize( resamplerSize );
	maskResampler->SetOutputSpacing( spacings2D() );
	
	// z scalers
	zScaler     = ZScaleType::New();
	maskZScaler = MaskZScaleType::New();
	zScaler    ->ChangeSpacingOn();
	maskZScaler->ChangeSpacingOn();
	zScaler    ->SetOutputSpacing( spacings );
	maskZScaler->SetOutputSpacing( spacings );
	
	// tile filter layout
	layout[0] = 1;
  layout[1] = 1;
  layout[2] = 0;
	
	// masks
	mask3D = MaskType3D::New();
}
	
void Stack::updateVolumes() {
  buildSlices();
  buildVolume();
  buildMaskSlices();
  buildMaskVolume();
}
	
void Stack::buildSlices() {
  for(unsigned int slice_number=0; slice_number<originalImages.size(); slice_number++) {
		// resample transformed image
		resampler->SetInput( originalImages[slice_number] );
		resampler->SetTransform( transforms[slice_number] );
		resampler->Update();
		
		// save output
    slices[slice_number] = resampler->GetOutput();
		slices[slice_number]->DisconnectPipeline();
	}
}

void Stack::buildVolume() {
  // construct tile filter
	tileFilter = TileFilterType::New();
	tileFilter->SetLayout( layout );
	
	// add new images to filter stack
  for(SliceVectorType::iterator it = slices.begin(); it != slices.end(); it++) {
		tileFilter->PushBackInput( *it );
	}
	
	zScaler->SetInput( tileFilter->GetOutput() );
	zScaler->Update();
	volume = zScaler->GetOutput();
}

void Stack::buildMaskSlices() {
	// make new 2D masks and assign mask slices to them      
	for(unsigned int slice_number=0; slice_number<GetSize(); slice_number++)
	{
    buildMaskSlice(slice_number);
	}
}

void Stack::buildMaskSlice(unsigned int slice_number) {
  // generate mask slice
	maskResampler->SetInput( original2DMasks[slice_number]->GetImage() );
	maskResampler->SetTransform( transforms[slice_number] );
	maskResampler->Update();
	
	// append mask to mask vector
  resampled2DMasks[slice_number]->SetImage( maskResampler->GetOutput() );
	// necessary to force resampler to make new pointer when updated
  maskResampler->GetOutput()->DisconnectPipeline();
}
	
void Stack::buildMaskVolume() {
  // construct tile filter
	maskTileFilter = MaskTileFilterType::New();
	maskTileFilter->SetLayout( layout );
	
  // append mask slices to the tile filter
	for(unsigned int i=0; i<originalImages.size(); i++)
	{
    maskTileFilter->PushBackInput( resampled2DMasks[i]->GetImage() );
	}
	
	maskZScaler->SetInput( maskTileFilter->GetOutput() );
	maskZScaler->Update();		
	mask3D->SetImage( maskZScaler->GetOutput() );
}

void Stack::checkSliceNumber(unsigned int slice_number) const {
  if( slice_number >= this->GetSize() ) {
    cerr << "Wait a minute, trying to access slice number bigger than this stack!" << endl;
    exit(EXIT_FAILURE);
  }
}
  
void Stack::ShrinkSliceMask(unsigned int slice_number) {
  // increment numberOfTimesTooBig
  numberOfTimesTooBig[slice_number]++;
  
  // initialise and allocate new mask image
  MaskSliceType::ConstPointer oldMaskSlice = original2DMasks[slice_number]->GetImage();
  MaskSliceType::RegionType region = oldMaskSlice->GetLargestPossibleRegion();
  MaskSliceType::Pointer newMaskSlice = MaskSliceType::New();
  newMaskSlice->SetRegions( region );
  newMaskSlice->CopyInformation( oldMaskSlice );
  newMaskSlice->Allocate();
  
  // make smaller image region
  MaskSliceType::RegionType::SizeType size;
  MaskSliceType::RegionType::IndexType index;
  unsigned int factor = pow(2.0, (int)numberOfTimesTooBig[slice_number]);
  
  for(unsigned int i=0; i<2; i++) {
    size[i] = resamplerSize[i] / factor;
    index[i] = (offset[i] / spacings[i]) + resamplerSize[i] * ( 1.0 - ( 1.0/factor ) ) / 2;
  }
  
  region.SetSize( size );
  region.SetIndex( index );
  
  // set pixels outside region to black and inside region to white
  newMaskSlice->FillBuffer( 0 );
  MaskSliceIteratorType it(newMaskSlice, region);
  for (it.GoToBegin(); !it.IsAtEnd(); ++it ) {
    it.Set(255);
  }
  
  // attach new image to mask
  original2DMasks[slice_number]->SetImage( newMaskSlice );
  buildMaskSlice(slice_number);
}

bool Stack::fileExists(const string& strFilename) { 
  struct stat stFileInfo; 
  bool blnReturn; 
  int intStat;

  // Attempt to get the file attributes 
  intStat = stat(strFilename.c_str(),&stFileInfo); 
  if(intStat == 0) { 
    // We were able to get the file attributes 
    // so the file obviously exists. 
    blnReturn = true; 
  } else { 
    // We were not able to get the file attributes. 
    // This may mean that we don't have permission to 
    // access the folder which contains this file. If you 
    // need to do that level of checking, lookup the 
    // return values of stat which will give you 
    // more details on why stat failed.
    blnReturn = false; 
  } 

  return(blnReturn); 
}

#endif