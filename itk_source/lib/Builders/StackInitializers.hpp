#ifndef STACKINITIALIZERS_HPP_
#define STACKINITIALIZERS_HPP_

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "Stack.hpp"
#include "Dirs.hpp"
#include "ScaleImages.hpp"

template <typename StackType>
boost::shared_ptr< StackType > InitializeLoResStack(typename StackType::SliceVectorType Images, const string& roi = "whole_heart")
{
  // get downsample ratio
  float DownsampleRatio;
  boost::shared_ptr<YAML::Node> downsample_ratios = config("downsample_ratios.yml");
  (*downsample_ratios)["LoRes"] >> DownsampleRatio;
  
  // get spacings
  typename StackType::VolumeType::SpacingType Spacings;
  for(unsigned int i=0; i<3; i++) {
    (*config("image_dimensions.yml"))["LoResSpacings"][i] >> Spacings[i];
  }
  
  // multiply in-plane spacings by downsample ratio
  for(unsigned int i=0; i<2; i++) {
    Spacings[i] *= DownsampleRatio;
  }
  
  // get 2D spacings to scale input images
  typename StackType::SliceType::SpacingType Spacings2D;
  Spacings2D[0] = Spacings[0];
  Spacings2D[1] = Spacings[1];
  
  // get size
  boost::shared_ptr<YAML::Node> roiNode = config("ROIs/" + roi + ".yml");
  typename StackType::SliceType::SizeType Size;
  for(unsigned int i=0; i<2; i++) {
    (*roiNode)["Size"][i] >> Size[i];
    Size[i] /= DownsampleRatio;
  }
  
  scaleImages< typename StackType::SliceType >(Images, Spacings2D);
  return boost::make_shared< StackType >(Images, Spacings, Size);
}

template <typename StackType>
boost::shared_ptr< StackType > InitializeHiResStack(typename StackType::SliceVectorType Images, const string& roi = "whole_heart")
{
  // get downsample ratios
  float LoResDownsampleRatio, HiResDownsampleRatio;
  boost::shared_ptr<YAML::Node> downsample_ratios = config("downsample_ratios.yml");
  (*downsample_ratios)["LoRes"] >> LoResDownsampleRatio;
  (*downsample_ratios)["HiRes"] >> HiResDownsampleRatio;
  
  // get spacings
  typename StackType::VolumeType::SpacingType LoResSpacings;
  typename StackType::SliceType::SpacingType HiResOriginalSpacings;
  for(unsigned int i=0; i<3; i++) (*config("image_dimensions.yml"))["LoResSpacings"][i] >> LoResSpacings[i];
  for(unsigned int i=0; i<2; i++) (*config("image_dimensions.yml"))["HiResSpacings"][i] >> HiResOriginalSpacings[i];
  
  // multiply in-plane spacings by downsample ratios
  for(unsigned int i=0; i<2; i++)
  {
    LoResSpacings[i]         *= LoResDownsampleRatio;
    HiResOriginalSpacings[i] *= HiResDownsampleRatio;
  }
  
  // get size
  boost::shared_ptr<YAML::Node> roiNode = config("ROIs/" + roi + ".yml");
  typename StackType::SliceType::SizeType LoResSize;
  for(unsigned int i=0; i<2; i++) {
    (*roiNode)["Size"][i] >> LoResSize[i];
    LoResSize[i] /= LoResDownsampleRatio;
  }
  
  scaleImages<typename StackType::SliceType>(Images, HiResOriginalSpacings);
  
  return boost::make_shared< StackType >(Images, LoResSpacings, LoResSize);
}


#endif