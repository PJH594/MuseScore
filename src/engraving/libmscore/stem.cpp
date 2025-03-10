/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "stem.h"

#include <cmath>

#include "draw/types/brush.h"

#include "beam.h"
#include "chord.h"
#include "hook.h"
#include "note.h"
#include "score.h"
#include "staff.h"
#include "stafftype.h"
#include "tremolo.h"

#include "log.h"

using namespace mu;
using namespace mu::draw;
using namespace mu::engraving;

static const ElementStyle stemStyle {
    { Sid::stemWidth, Pid::LINE_WIDTH }
};

Stem::Stem(Chord* parent)
    : EngravingItem(ElementType::STEM, parent)
{
    initElementStyle(&stemStyle);
    resetProperty(Pid::USER_LEN);
}

EngravingItem* Stem::elementBase() const
{
    return parentItem();
}

staff_idx_t Stem::vStaffIdx() const
{
    return staffIdx() + chord()->staffMove();
}

bool Stem::up() const
{
    return chord() ? chord()->up() : true;
}

void Stem::setBaseLength(Millimetre baseLength)
{
    m_baseLength = Millimetre(std::abs(baseLength.val()));
    renderer()->layoutItem(this);
}

void Stem::spatiumChanged(double oldValue, double newValue)
{
    m_userLength = (m_userLength / oldValue) * newValue;
    renderer()->layoutItem(this);
}

//! In chord coordinates
PointF Stem::flagPosition() const
{
    return pos() + PointF(bbox().left(), up() ? -length() : length());
}

std::vector<mu::PointF> Stem::gripsPositions(const EditData&) const
{
    return { pagePos() + m_line.p2() };
}

void Stem::startEdit(EditData& ed)
{
    EngravingItem::startEdit(ed);
    ElementEditDataPtr eed = ed.getData(this);
    eed->pushProperty(Pid::USER_LEN);
}

void Stem::startEditDrag(EditData& ed)
{
    EngravingItem::startEditDrag(ed);
    ElementEditDataPtr eed = ed.getData(this);
    eed->pushProperty(Pid::USER_LEN);
}

void Stem::editDrag(EditData& ed)
{
    double yDelta = ed.delta.y();
    m_userLength += up() ? Millimetre(-yDelta) : Millimetre(yDelta);
    renderer()->layoutItem(this);
    Chord* c = chord();
    if (c->hook()) {
        c->hook()->move(PointF(0.0, yDelta));
    }
}

void Stem::reset()
{
    undoChangeProperty(Pid::USER_LEN, Millimetre(0.0));
    EngravingItem::reset();
}

bool Stem::acceptDrop(EditData& data) const
{
    EngravingItem* e = data.dropElement;
    if ((e->type() == ElementType::TREMOLO) && (toTremolo(e)->tremoloType() <= TremoloType::R64)) {
        return true;
    }
    return false;
}

EngravingItem* Stem::drop(EditData& data)
{
    EngravingItem* e = data.dropElement;
    Chord* ch  = chord();

    switch (e->type()) {
    case ElementType::TREMOLO:
        toTremolo(e)->setParent(ch);
        undoAddElement(e);
        return e;
    default:
        delete e;
        break;
    }
    return 0;
}

PropertyValue Stem::getProperty(Pid propertyId) const
{
    switch (propertyId) {
    case Pid::LINE_WIDTH:
        return lineWidth();
    case Pid::USER_LEN:
        return userLength();
    case Pid::STEM_DIRECTION:
        return PropertyValue::fromValue<DirectionV>(chord()->stemDirection());
    default:
        return EngravingItem::getProperty(propertyId);
    }
}

bool Stem::setProperty(Pid propertyId, const PropertyValue& v)
{
    switch (propertyId) {
    case Pid::LINE_WIDTH:
        setLineWidth(v.value<Millimetre>());
        break;
    case Pid::USER_LEN:
        setUserLength(v.value<Millimetre>());
        break;
    case Pid::STEM_DIRECTION:
        chord()->setStemDirection(v.value<DirectionV>());
        break;
    default:
        return EngravingItem::setProperty(propertyId, v);
    }
    triggerLayout();
    return true;
}

PropertyValue Stem::propertyDefault(Pid id) const
{
    switch (id) {
    case Pid::USER_LEN:
        return 0.0;
    case Pid::STEM_DIRECTION:
        return PropertyValue::fromValue<DirectionV>(DirectionV::AUTO);
    default:
        return EngravingItem::propertyDefault(id);
    }
}
