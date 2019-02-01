#pragma once
#include <map>
#include <string>
#include <functional>
#include <memory>

template <typename P, typename... Args>
struct AbstractFactory
{
	template<typename T>
	struct register_t
	{
		register_t(const std::string& key)
		{
			AbstractFactory<P, Args...>::get().m_map.emplace(key, [](Args&&... args)
            {
                return new T(std::forward<Args>(args)...);
            });
		}

		register_t(const std::string& key, std::function<P*(Args&&...)>&& f)
		{
			AbstractFactory<P, Args...>::get().m_map.emplace(key, std::move(f));
		}
	};

	inline P* produce(const std::string& key, Args&&... args)
	{
		if (m_map.find(key) == m_map.end())
			throw std::invalid_argument("the message key is not exist!");

		return m_map[key](std::forward<Args>(args)...);
	}

	inline static AbstractFactory<P, Args...>& get()
	{
		static AbstractFactory<P, Args...> instance;
		return instance;
	}

private:
	AbstractFactory<P, Args...>() {};
	AbstractFactory<P, Args...>(const AbstractFactory<P, Args...>&) = delete;
	AbstractFactory<P, Args...>(AbstractFactory<P, Args...>&&) = delete;

	std::map<std::string, std::function<P*(Args&&...)>> m_map;
};

#define REGISTER_FACTORY(T, key, ...) static AbstractFactory<T, __VA_ARGS__>::register_t<T> REGISTER_FACTORY_VNAME(T)(key);
#define REGISTER_FACTORY_CREATER(T, key, f, ...) static AbstractFactory<T, __VA_ARGS__>::register_t<T> REGISTER_FACTORY_VNAME(T)(key, f);

#define REGISTER_FACTORY_VNAME(T) reg_factory_##T##_