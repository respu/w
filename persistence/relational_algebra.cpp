#include <persistence/relational_algebra.hpp>

namespace persistence {
  namespace relational_algebra {
    using wayward::make_cloning_ptr;

    Projection projection(std::string relation) {
      return Projection{relation};
    }

    Value column(std::string relation, std::string column) {
      return Value {
        make_cloning_ptr(
          new ast::ColumnReference{
            std::move(relation),
            std::move(column)
          }
        )};
    }

    Value literal(double n) {
      return Value {
        make_cloning_ptr(new ast::NumericLiteral{n})
      };
    }

    SQL sql(std::string sql) {
      return SQL{std::move(sql)};
    }

    Condition operator&&(Condition&& lhs, Condition&& rhs) {
      return Condition{
        make_cloning_ptr(new ast::LogicalCondition{
          std::move(lhs.cond),
          std::move(rhs.cond),
          ast::LogicalCondition::AND
        })
      };
    }

    Condition operator||(Condition&& lhs, Condition&& rhs) {
      return Condition{
        make_cloning_ptr(new ast::LogicalCondition{
          std::move(lhs.cond),
          std::move(rhs.cond),
          ast::LogicalCondition::OR
        })
      };
    }

    Value::Value(SQL sql) {
      value = make_cloning_ptr(new ast::SQLFragmentValue{std::move(sql.sql)});
    }

    Condition Value::like(std::string literal) && {
      return Condition{make_cloning_ptr(new ast::BinaryCondition{
        std::move(value),
        make_cloning_ptr(new ast::StringLiteral{literal}),
        ast::BinaryCondition::Like
      })};
    }

    Condition Value::operator==(Value&& other) && {
      return Condition{make_cloning_ptr(new ast::BinaryCondition{
        std::move(value),
        std::move(other.value),
        ast::BinaryCondition::Eq
      })};
    }

    Condition Value::operator<(Value&& other) && {
      return Condition{make_cloning_ptr(new ast::BinaryCondition{
        std::move(value),
        std::move(other.value),
        ast::BinaryCondition::LessThan
      })};
    }

    Condition Value::operator>(Value&& other) && {
      return Condition{make_cloning_ptr(new ast::BinaryCondition{
        std::move(value),
        std::move(other.value),
        ast::BinaryCondition::GreaterThan
      })};
    }

    Condition Value::operator<=(Value&& other) && {
      return Condition{make_cloning_ptr(new ast::BinaryCondition{
        std::move(value),
        std::move(other.value),
        ast::BinaryCondition::LessThanOrEq
      })};
    }

    Condition Value::operator>=(Value&& other) && {
      return Condition{make_cloning_ptr(new ast::BinaryCondition{
        std::move(value),
        std::move(other.value),
        ast::BinaryCondition::GreaterThanOrEq
      })};
    }

    Projection::Projection(std::string relation) {
      query = make_cloning_ptr(new ast::SelectQuery);
      query->relation = std::move(relation);
    }

    Projection Projection::where(Condition cond) const& {
      Projection p = *this;
      return std::move(p).where(std::move(cond));
    }

    Projection Projection::where(Condition cond) && {
      if (query->where) {
        auto new_cond = make_cloning_ptr(new ast::LogicalCondition{
          std::move(query->where),
          std::move(cond.cond),
          ast::LogicalCondition::AND
        });
        query->where = std::move(new_cond);
      } else {
        query->where = std::move(cond.cond);
      }
      return std::move(*this);
    }

    Projection Projection::order(std::vector<Ordering> ordering_values) const& {
      Projection p = *this;
      return std::move(p).order(std::move(ordering_values));
    }

    Projection Projection::order(std::vector<Ordering> ordering_values) && {
      query->order.clear();
      for (auto& ord: ordering_values) {
        query->order.push_back({std::move(ord.value.value), ord.ordering});
      }
      return std::move(*this);
    }

    Projection Projection::reverse_order(bool b) const& {
      Projection p = *this;
      return std::move(p).reverse_order(b);
    }

    Projection Projection::reverse_order(bool b) && {
      for (auto& ord: query->order) {
        switch (ord.ordering) {
          case ast::Ordering::Ascending: {
            ord.ordering = ast::Ordering::Descending;
            break;
          }
          case ast::Ordering::Descending: {
            ord.ordering = ast::Ordering::Ascending;
            break;
          }
        }
      }
      return std::move(*this);
    }

    Projection Projection::limit(size_t n) && {
      query->limit = n;
      return std::move(*this);
    }

    Projection Projection::limit(size_t n) const& {
      Projection p = *this;
      return std::move(p).limit(n);
    }

    Projection Projection::select(std::vector<SelectAlias> selects) && {
      query->select.resize(selects.size());
      for (size_t i = 0; i < selects.size(); ++i) {
        query->select[i].value = std::move(selects[i].value.value);
        query->select[i].alias = std::move(selects[i].alias);
      }
      return std::move(*this);
    }

    Projection Projection::select(std::vector<SelectAlias> selects) const& {
      Projection p = *this;
      return std::move(p).select(std::move(selects));
    }

    Projection Projection::left_join(std::string relation, std::string as, Condition on) && {
      query->joins.push_back(make_cloning_ptr(new ast::Join{
        ast::Join::LeftOuter,
        std::move(relation),
        std::move(as),
        std::move(on.cond)
      }));
      return std::move(*this);
    }
  }
}
