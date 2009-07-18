/* Copyright (C) 2003-2009 Jesper K. Pedersen <blackie@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "ThumbnailComponent.h"
#include "ThumbnailFactory.h"

ThumbnailView::ThumbnailComponent::ThumbnailComponent( ThumbnailFactory* factory )
    :_factory( factory )
{
}

ThumbnailView::ThumbnailModel* ThumbnailView::ThumbnailComponent::model()
{
    return _factory->model();
}

ThumbnailView::CellGeometry* ThumbnailView::ThumbnailComponent::cellGeometryInfo()
{
    return _factory->cellGeometry();
}

ThumbnailView::ThumbnailWidget* ThumbnailView::ThumbnailComponent::widget()
{
    return _factory->widget();
}

ThumbnailView::ThumbnailPainter* ThumbnailView::ThumbnailComponent::painter()
{
    return _factory->painter();
}

const ThumbnailView::ThumbnailModel* ThumbnailView::ThumbnailComponent::model() const
{
    return _factory->model();
}

const ThumbnailView::CellGeometry* ThumbnailView::ThumbnailComponent::cellGeometryInfo() const
{
    return _factory->cellGeometry();
}

const ThumbnailView::ThumbnailWidget* ThumbnailView::ThumbnailComponent::widget() const
{
    return _factory->widget();
}

const ThumbnailView::ThumbnailPainter* ThumbnailView::ThumbnailComponent::painter() const
{
    return _factory->painter();
}

ThumbnailView::ThumbnailCache* ThumbnailView::ThumbnailComponent::cache()
{
    return _factory->cache();
}

const ThumbnailView::ThumbnailCache* ThumbnailView::ThumbnailComponent::cache() const
{
    return _factory->cache();
}
