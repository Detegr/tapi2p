class Event
{
	public:
		enum Type
		{
			Message
		};
	private:
		Type m_Type;
	public:
		Event(Type t) : m_Type(t) {}
};
