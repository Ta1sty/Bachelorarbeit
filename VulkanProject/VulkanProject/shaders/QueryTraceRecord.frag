struct QueryTrace {
	mat4 matrix;
	vec3 start;
	float t;
	vec3 end;
	uint nodeNumber;
	uint isValid; // valid if 1, otherwise invalid
	int nodeLevel; // the nodeNumber for this
	uint triangleIntersections;
	uint instanceIntersections;
};

layout(binding = TRACE_BINDING, set = 3) buffer QueryTraceBuffer { QueryTrace[] queryTraces; };

bool recordTrace = false;
uint nextRecord = 0;
void startTraceRecord() {
	ivec2 xy = ivec2(gl_FragCoord.xy);
	if (recordQueryTrace && xy.x == pixelX && xy.y == pixelY) {
		recordTrace = true;
		nextRecord = 0;
		queryTraces[0].isValid = 1;
	}
}
void recordQuery(uint nodeNumber, int nodeLevel, float t, vec3 start, vec3 end, uint triangleIntersections, uint instanceIntersections) { // records the 
	if (!recordTrace || nextRecord >= traceMax - 2) return;
	QueryTrace add;
	add.start = start;
	add.end = end;
	add.nodeLevel = nodeLevel;
	add.nodeNumber = nodeNumber;
	add.isValid = 1;
	add.triangleIntersections = triangleIntersections;
	add.instanceIntersections = instanceIntersections;
	add.t = t;

	// add
	queryTraces[nextRecord] = add;
	nextRecord++;
}
void recordLODSelect(int node, mat4 world_to_object, float tNear, float rObject, float rWorld, float rPixel, float eigMax, int lod) {
	if (!recordTrace || nextRecord >= traceMax - 2) return;
	QueryTrace add;
	add.matrix = world_to_object;
	add.start = vec3(rObject, rWorld, rPixel);
	add.end = vec3(eigMax, 0, 0);
	add.nodeLevel = lod;
	add.nodeNumber = node;
	add.isValid = 1;
	add.triangleIntersections = 123;
	add.instanceIntersections = 321;
	add.t = tNear;
	// add
	queryTraces[nextRecord] = add;
	nextRecord++;
}
void checkQueryTrace(vec3 origin, vec3 direction) {
	if (!debug || !displayQueryTrace) return;
	int index = -1;
	for (int i = 0; i < traceMax; i++) {
		QueryTrace trace = queryTraces[i];
		if (trace.isValid != 1) break; // is valid?
		if (displayByLevel) {
			if (trace.nodeLevel / 2 != selectedLevel)
				continue;
		}
		vec3 E1 = normalize(trace.end - trace.start);
		vec3 E2 = normalize(direction);
		vec3 N = cross(E2, E1);
		float d = dot(N, trace.start - origin);
		if (abs(d) > 0.005f) continue; // intersection is not near enough
		mat3 M = mat3(E1, -E2, N);
		mat3 MInv = inverse(M);
		vec3 ts = MInv * (origin - trace.start);
		float maxT = length(trace.end - trace.start);
		if (ts[0] < 0 || ts[1]<0 || ts[0] > maxT) continue; // outide of bounds
		vec3 P = trace.start + ts[0] * E1;
		float dst = length(P - origin);
		if (dst < 0.005 || dst > 50) continue; // to close to the display or to far away
		index = i;
	}
	if (index != -1) {
		SceneNode node = nodes[queryTraces[index].nodeNumber];
		SetDebugHsv(displayQueryTrace, node.TlasNumber, colorSensitivity, false);
	}
}
void endRecord() {
	if (!recordTrace) return;
	// adds an invalid record at the end of the list
	QueryTrace end;
	end.isValid = 0;
	queryTraces[nextRecord] = end;
}