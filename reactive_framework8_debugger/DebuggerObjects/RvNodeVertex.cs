namespace reactive_framework8_debugger
{
	public class RvNodeVertex
	{
		public string ID { get; private set; }

		public RvNodeVertex(string id_)
		{
			ID = id_;
		}

		public override string ToString()
		{
			return ID;
		}
    }
}