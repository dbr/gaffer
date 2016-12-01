//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2011-2013, Image Engine Design Inc. All rights reserved.
//  Copyright (c) 2012, John Haddon. All rights reserved.
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

#include "boost/bind.hpp"
#include "boost/bind/placeholders.hpp"

#include "IECore/SimpleTypedData.h"

#include "Gaffer/Plug.h"
#include "Gaffer/MetadataAlgo.h"

#include "GafferUI/CompoundNodule.h"
#include "GafferUI/LinearContainer.h"

using namespace std;
using namespace Imath;
using namespace IECore;
using namespace Gaffer;
using namespace GafferUI;

//////////////////////////////////////////////////////////////////////////
// Internal utilities
//////////////////////////////////////////////////////////////////////////

namespace
{

IECore::InternedString g_orientationKey( "compoundNodule:orientation"  );
IECore::InternedString g_spacingKey( "compoundNodule:spacing"  );
IECore::InternedString g_directionKey( "compoundNodule:direction"  );

LinearContainer::Orientation orientationMetadata( const Plug *plug, LinearContainer::Orientation defaultValue )
{
	ConstStringDataPtr orientationData = Metadata::value<StringData>( plug, g_orientationKey );
	if( !orientationData )
	{
		return defaultValue;
	}

	if( orientationData->readable() == "x" )
	{
		return LinearContainer::X;
	}
	else if( orientationData->readable() == "y" )
	{
		return LinearContainer::Y;
	}
	else
	{
		return LinearContainer::Z;
	}
}

float spacingMetadata( const Plug *plug, float defaultValue )
{
	ConstFloatDataPtr spacingData = Metadata::value<FloatData>( plug, g_spacingKey );
	return spacingData ? spacingData->readable() : defaultValue;
}

LinearContainer::Direction directionMetadata( const Plug *plug, LinearContainer::Direction defaultValue )
{
	ConstStringDataPtr directionData = Metadata::value<StringData>( plug, g_directionKey );
	if( !directionData )
	{
		return defaultValue;
	}

	return directionData->readable() == "increasing" ? LinearContainer::Increasing : LinearContainer::Decreasing;
}

} // namespace

//////////////////////////////////////////////////////////////////////////
// CompoundNodule
//////////////////////////////////////////////////////////////////////////

IE_CORE_DEFINERUNTIMETYPED( CompoundNodule );

Nodule::NoduleTypeDescription<CompoundNodule> CompoundNodule::g_noduleTypeDescription;

CompoundNodule::CompoundNodule( Gaffer::PlugPtr plug, LinearContainer::Orientation orientation,
	float spacing, LinearContainer::Direction direction )
	:	Nodule( plug )
{
	orientation = orientationMetadata( plug.get(), orientation );
	spacing = spacingMetadata( plug.get(), spacing );
	direction = directionMetadata( plug.get(), direction );

	if( direction == LinearContainer::InvalidDirection )
	{
		direction = orientation == LinearContainer::X ? LinearContainer::Increasing : LinearContainer::Decreasing;
	}

	m_row = new LinearContainer( "row", orientation, LinearContainer::Centre, spacing, direction );
	addChild( m_row );

	for( Gaffer::PlugIterator it( plug.get() ); !it.done(); ++it )
	{
		NodulePtr nodule = Nodule::create( *it );
		if( nodule )
		{
			m_row->addChild( nodule );
		}
	}

	plug->childAddedSignal().connect( boost::bind( &CompoundNodule::childAdded, this, ::_1,  ::_2 ) );
	plug->childRemovedSignal().connect( boost::bind( &CompoundNodule::childRemoved, this, ::_1,  ::_2 ) );

	Metadata::plugValueChangedSignal().connect( boost::bind( &CompoundNodule::plugMetadataChanged, this, ::_1, ::_2, ::_3, ::_4 ) );
}

CompoundNodule::~CompoundNodule()
{
}

Imath::Box3f CompoundNodule::bound() const
{
	return m_row->bound();
}

void CompoundNodule::doRender( const Style *style ) const
{
	m_row->render( style );
}

bool CompoundNodule::acceptsChild( const Gaffer::GraphComponent *potentialChild ) const
{
	return children().size()==0;
}

Nodule *CompoundNodule::nodule( const Gaffer::Plug *plug )
{
	for( NoduleIterator it( m_row.get() ); !it.done(); ++it )
	{
		if( (*it)->plug() == plug )
		{
			return it->get();
		}
	}
	return 0;
}

const Nodule *CompoundNodule::nodule( const Gaffer::Plug *plug ) const
{
	// preferring the nasty casts over mainaining two nearly identical implementations
	return const_cast<CompoundNodule *>( this )->nodule( plug );
}

void CompoundNodule::childAdded( Gaffer::GraphComponent *parent, Gaffer::GraphComponent *child )
{
	Gaffer::Plug *plug = IECore::runTimeCast<Gaffer::Plug>( child );
	if( !plug )
	{
		return;
	}

	if( nodule( plug ) )
	{
		return;
	}

	NodulePtr nodule = Nodule::create( plug );
	if( nodule )
	{
		m_row->addChild( nodule );
	}
}

void CompoundNodule::childRemoved( Gaffer::GraphComponent *parent, Gaffer::GraphComponent *child )
{
	Gaffer::Plug *plug = IECore::runTimeCast<Gaffer::Plug>( child );
	if( !plug )
	{
		return;
	}

	for( NoduleIterator it( m_row.get() ); !it.done(); ++it )
	{
		if( (*it)->plug() == plug )
		{
			m_row->removeChild( *it );
			break;
		}
	}
}

void CompoundNodule::plugMetadataChanged( IECore::TypeId nodeTypeId, const Gaffer::MatchPattern &plugPath, IECore::InternedString key, const Gaffer::Plug *plug )
{
	if( !affectedByChange( this->plug(), nodeTypeId, plugPath, plug ) )
	{
		return;
	}

	if( key == g_orientationKey )
	{
		m_row->setOrientation( orientationMetadata( this->plug(), LinearContainer::X ) );
	}
	else if( key == g_spacingKey )
	{
		m_row->setSpacing( spacingMetadata( this->plug(), 0 ) );
	}

	if( key == g_directionKey || key == g_orientationKey )
	{
		m_row->setDirection(
			directionMetadata(
				this->plug(),
				m_row->getOrientation() == LinearContainer::X ? LinearContainer::Increasing : LinearContainer::Decreasing
			)
		);
	}
}
