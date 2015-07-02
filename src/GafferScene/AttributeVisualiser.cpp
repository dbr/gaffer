//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2015, Image Engine Design Inc. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//      * Redistributions of source code must retain the above
//        copyright notice, this list of conditions and the following
//        disclaimer.
//
//      * Redistributions in binary form must reproduce the above
//        copyright notice, this list of conditions and the following
//        disclaimer in the documentation and/or other materials provided with
//        the distribution.
//
//      * Neither the name of John Haddon nor the names of
//        any other contributors to this software may be used to endorse or
//        promote products derived from this software without specific prior
//        written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////

#include "OpenEXR/ImathRandom.h"

#include "IECore/Shader.h"

#include "Gaffer/StringPlug.h"

#include "GafferScene/AttributeVisualiser.h"

using namespace Imath;
using namespace IECore;
using namespace Gaffer;
using namespace GafferScene;

IE_CORE_DEFINERUNTIMETYPED( AttributeVisualiser );

size_t AttributeVisualiser::g_firstPlugIndex = 0;

AttributeVisualiser::AttributeVisualiser( const std::string &name )
	:	SceneElementProcessor( name, Filter::EveryMatch )
{
	storeIndexOfNextChild( g_firstPlugIndex );

	addChild( new StringPlug( "attributeName" ) );

	addChild( new IntPlug( "mode", Plug::In, Color, Color, ShaderNodeColor ) );
	addChild( new FloatPlug( "min", Plug::In, 0.0f ) );
	addChild( new FloatPlug( "max", Plug::In, 1.0f ) );

	SplinefColor3f rampDefault;
	rampDefault.points.insert( SplinefColor3f::Point( 1.0f, Color3f( 0.0f, 1.0f, 0.0f ) ) );
	rampDefault.points.insert( SplinefColor3f::Point( 1.0f, Color3f( 0.0f, 1.0f, 0.0f ) ) );
	rampDefault.points.insert( SplinefColor3f::Point( 0.0f, Color3f( 1.0f, 0.0f, 0.0f ) ) );
	rampDefault.points.insert( SplinefColor3f::Point( 0.0f, Color3f( 1.0f, 0.0f, 0.0f ) ) );
	addChild( new SplinefColor3fPlug( "ramp", Plug::In, rampDefault ) );

	addChild( new StringPlug( "shaderType", Plug::In, "gl:surface" ) );
	addChild( new StringPlug( "shaderName", Plug::In, "Constant" ) );
	addChild( new StringPlug( "shaderParameter", Plug::In, "Cs" ) );

	// Fast pass-throughs for the things we don't alter.
	outPlug()->objectPlug()->setInput( inPlug()->objectPlug() );
	outPlug()->transformPlug()->setInput( inPlug()->transformPlug() );
	outPlug()->boundPlug()->setInput( inPlug()->boundPlug() );
}

AttributeVisualiser::~AttributeVisualiser()
{
}

Gaffer::StringPlug *AttributeVisualiser::attributeNamePlug()
{
	return getChild<StringPlug>( g_firstPlugIndex );
}

const Gaffer::StringPlug *AttributeVisualiser::attributeNamePlug() const
{
	return getChild<StringPlug>( g_firstPlugIndex );
}

Gaffer::IntPlug *AttributeVisualiser::modePlug()
{
	return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

const Gaffer::IntPlug *AttributeVisualiser::modePlug() const
{
	return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

Gaffer::FloatPlug *AttributeVisualiser::minPlug()
{
	return getChild<FloatPlug>( g_firstPlugIndex + 2 );
}

const Gaffer::FloatPlug *AttributeVisualiser::minPlug() const
{
	return getChild<FloatPlug>( g_firstPlugIndex + 2 );
}

Gaffer::FloatPlug *AttributeVisualiser::maxPlug()
{
	return getChild<FloatPlug>( g_firstPlugIndex + 3 );
}

const Gaffer::FloatPlug *AttributeVisualiser::maxPlug() const
{
	return getChild<FloatPlug>( g_firstPlugIndex + 3 );
}

Gaffer::SplinefColor3fPlug *AttributeVisualiser::rampPlug()
{
	return getChild<SplinefColor3fPlug>( g_firstPlugIndex + 4 );
}

const Gaffer::SplinefColor3fPlug *AttributeVisualiser::rampPlug() const
{
	return getChild<SplinefColor3fPlug>( g_firstPlugIndex + 4 );
}

Gaffer::StringPlug *AttributeVisualiser::shaderTypePlug()
{
	return getChild<StringPlug>( g_firstPlugIndex + 5 );
}

const Gaffer::StringPlug *AttributeVisualiser::shaderTypePlug() const
{
	return getChild<StringPlug>( g_firstPlugIndex + 5 );
}

Gaffer::StringPlug *AttributeVisualiser::shaderNamePlug()
{
	return getChild<StringPlug>( g_firstPlugIndex + 6 );
}

const Gaffer::StringPlug *AttributeVisualiser::shaderNamePlug() const
{
	return getChild<StringPlug>( g_firstPlugIndex + 6 );
}

Gaffer::StringPlug *AttributeVisualiser::shaderParameterPlug()
{
	return getChild<StringPlug>( g_firstPlugIndex + 7 );
}

const Gaffer::StringPlug *AttributeVisualiser::shaderParameterPlug() const
{
	return getChild<StringPlug>( g_firstPlugIndex + 7 );
}

void AttributeVisualiser::affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const
{
	SceneElementProcessor::affects( input, outputs );

	if(
		input == attributeNamePlug() ||
		input == modePlug() ||
		input == minPlug() ||
		input == maxPlug() ||
		input == shaderTypePlug() ||
		input == shaderNamePlug() ||
		input == shaderParameterPlug() ||
		rampPlug()->isAncestorOf( input )
	)
	{
		outputs.push_back( outPlug()->attributesPlug() );
	}
}

bool AttributeVisualiser::processesAttributes() const
{
	return true;
}

void AttributeVisualiser::hashProcessedAttributes( const ScenePath &path, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	attributeNamePlug()->hash( h );
	modePlug()->hash( h );
	minPlug()->hash( h );
	maxPlug()->hash( h );
	rampPlug()->hash( h );
	shaderTypePlug()->hash( h );
	shaderNamePlug()->hash( h );
	shaderParameterPlug()->hash( h );
}

IECore::ConstCompoundObjectPtr AttributeVisualiser::computeProcessedAttributes( const ScenePath &path, const Gaffer::Context *context, IECore::ConstCompoundObjectPtr inputAttributes ) const
{
	const std::string attributeName = attributeNamePlug()->getValue();
	if( !attributeName.size() )
	{
		return inputAttributes;
	}

	const std::string shaderType = shaderTypePlug()->getValue();
	if( !shaderType.size() )
	{
		return inputAttributes;
	}

	const Object *attribute = inputAttributes->member<Object>( attributeName );
	if( !attribute )
	{
		if( !inputAttributes->member<Object>( shaderType ) )
		{
			return inputAttributes;
		}
	}

	CompoundObjectPtr result = new CompoundObject;
	// Since we're not going to modify any existing members (only add a new one),
	// and our result becomes const on returning it, we can directly reference
	// the input members in our result without copying. Be careful not to modify
	// them though!
	result->members() = inputAttributes->members();

	if( !attribute )
	{
		result->members().erase( shaderType );
		return result;
	}

	// Compute our colour.

	Color3f color( 0.0f );
	const Mode mode = (Mode)modePlug()->getValue();
	if( mode == Random )
	{
		Rand32 r( tbb_hasher( attribute->hash() ) );
		for( int i = 0; i < 3; ++i )
		{
			color[i] = r.nextf();
		}
	}
	else if( mode == ShaderNodeColor )
	{
		const Shader *shader = runTimeCast<const Shader>( attribute );
		if( !shader )
		{
			if( const ObjectVector *objectVector = runTimeCast<const ObjectVector>( attribute ) )
			{
				if( objectVector->members().size() )
				{
					shader = runTimeCast<const Shader>( objectVector->members()[0].get() );
				}
			}
		}
		if( shader )
		{
			const Color3fData *colorData = shader->blindData()->member<const Color3fData>( "gaffer:nodeColor" );
			if( colorData )
			{
				color = colorData->readable();
			}
		}
	}
	else
	{
		// Color or FalseColor
		switch( attribute->typeId() )
		{
			case FloatDataTypeId :
				color = Color3f( static_cast<const FloatData *>( attribute )->readable() );
				break;
			case DoubleDataTypeId :
				color = Color3f( static_cast<const DoubleData *>( attribute )->readable() );
				break;
			case IntDataTypeId :
				color = Color3f( static_cast<const IntData *>( attribute )->readable() );
				break;
			case BoolDataTypeId :
				color = Color3f( static_cast<const BoolData *>( attribute )->readable() );
				break;
			default :
				throw IECore::Exception( boost::str(
					boost::format( "Unsupported attribute data type \"%s\"" ) % attribute->typeName()
				) );
		}
		const Color3f min( minPlug()->getValue() );
		const Color3f max( maxPlug()->getValue() );
		color = ( color - min ) / ( max - min );
		if( mode == FalseColor )
		{
			const SplinefColor3f ramp = rampPlug()->getValue();
			color = ramp( color[0] );
		}
	}

	// Apply the colour using a shader.

	ShaderPtr shader = new Shader( shaderNamePlug()->getValue(), shaderType );
	shader->parameters()[shaderParameterPlug()->getValue()] = new Color3fData( color );

	result->members()[shaderType] = shader;

	return result;
}