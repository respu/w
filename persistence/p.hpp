#pragma once
#ifndef P_HPP_INCLUDED
#define P_HPP_INCLUDED

#include <persistence/type.hpp>
#include <persistence/association.hpp>
#include <persistence/record_type.hpp>
#include <persistence/record_type_builder.hpp>
#include <persistence/persistence_macro.hpp>
#include <persistence/primary_key.hpp>
#include <persistence/connection.hpp>
#include <persistence/relational_algebra.hpp>
#include <persistence/projection.hpp>
#include <persistence/types.hpp>
#include <persistence/belongs_to.hpp>
#include <persistence/has_many.hpp>
#include <persistence/has_one.hpp>
#include <persistence/record.hpp>

#if !defined(PERSISTENCE_NO_SHORTHAND_NAMESPACE)
namespace p = persistence;
#endif

#endif
