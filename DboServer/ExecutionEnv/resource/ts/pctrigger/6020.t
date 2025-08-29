CNtlTSTrigger
{
	sm = 0;
	sq = 1;
	rq = 0;
	tid = 6020;
	title = -1;

	CNtlTSGroup
	{
		gid = 0;

		CDboTSContGAct
		{
			cid = 2;
			elnk = 255;
			nextlnk = 254;
			prelnk = "0;";

			CDboTSActPortal
			{
				dx = "-0.102";
				px = "-355.040";
				widx = 960000;
				dy = "0.00";
				py = "167.158";
				taid = 1;
				type = 1;
				dz = "64.288";
				pz = "0.995";
			}
		}
		CDboTSContEnd
		{
			cid = 254;
			prelnk = "2;";
			type = 0;
		}
		CDboTSContStart
		{
			cid = 0;
			stdiag = 0;
			nolnk = 255;
			rm = 0;
			yeslnk = 2;

			CDboTSColObject
			{
				objidx = "13;";
				widx = 960001;
			}
		}
	}
}