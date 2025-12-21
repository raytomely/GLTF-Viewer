#ifndef GLTF_LOADER_H
#define GLTF_LOADER_H

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

typedef struct
{
    float x, y, z, w;
}Vec4;

typedef struct
{
    float x,y,z;
}Vec3;

typedef struct
{
    float x,y;
}Vec2;

typedef struct
{
    glm::mat4 trans;
    glm::mat4 rot;
    glm::mat4 scale;
}TRS_Transform;

typedef struct
{
    unsigned int VAO, VBO[2], EBO;
    unsigned int vertices_count;
    unsigned int vertices_size;
    unsigned int texcoord_count;
    unsigned int texcoord_size;
    Vec3* vertices;
    Vec2* texcoord;
    glm::mat4 bone_matrix;
}Mesh_Data;

typedef struct Animation_Data Animation_Data;
typedef glm::mat4 (*Interpolate_Animation)(Animation_Data*, float);

typedef struct Animation_Node
{
    Mesh_Data* mesh;
    TRS_Transform trs;
    glm::mat4 local_transform;
    glm::mat4 global_transform;
    Animation_Node* parent;
	Animation_Node** children;
	Animation_Data* trans_anim;
	Animation_Data* rot_anim;
	Animation_Data* scale_anim;
	unsigned int children_count;
}Animation_Node;

typedef struct Animation_Data
{
    int type;
    int count;
    float *time;
    float *trs;
    Animation_Node* target_node;
    Interpolate_Animation interpolate_animation;
}Animation_Data;

typedef struct
{
    float currrent_time;
    float delta_time;
    float duration;
    float ticks_per_second;
}Animation_Timer;

typedef struct
{
    unsigned int anim_data_count;
    Animation_Data** anim_data;
    float duration;
}Model_Animation;

typedef struct
{
    unsigned int meshes_count;
    Mesh_Data** meshes;
    unsigned int texture;
    Animation_Node** anim_nodes;
    unsigned int anim_nodes_count;
    Animation_Node** root_nodes;
    unsigned int root_nodes_count;
    Model_Animation** animations;
    unsigned int animations_count;
    Model_Animation* curren_animation;
    float animation_time;
}Model_Data;

typedef struct
{
    Vec4* weights;
    Vec4* joints;
    unsigned int weights_count;
    unsigned int joints_count;
    unsigned int weights_size;
    unsigned int joints_size;
}Bone_Data;

typedef struct
{
    unsigned int meshes_count;
    Mesh_Data* meshes;
}Morph_Target_Data;

size_t float_count(cgltf_accessor* accessor)
{
    cgltf_size floats_per_element = cgltf_num_components(accessor->type);
	cgltf_size available_floats = accessor->count * floats_per_element;
	return available_floats;
}

#define float_buffer_size(accessor) float_count(accessor) * sizeof(float)

size_t index_count(cgltf_accessor* accessor)
{
    cgltf_size numbers_per_element = cgltf_num_components(accessor->type);
	cgltf_size available_numbers = accessor->count * numbers_per_element;
	return available_numbers;
}

#define index_buffer_size(accessor) index_count(accessor) * sizeof(int)

float* read_accessor(cgltf_accessor* accessor)
{
    size_t available_floats = float_count(accessor);
    //printf("available_floats=%d\n",available_floats);
    int mem_size = cgltf_accessor_unpack_floats(accessor, NULL, available_floats) * sizeof(float);
    float* float_buffer = (float*)malloc(mem_size);
    cgltf_accessor_unpack_floats(accessor, float_buffer, available_floats);
    //printf("mem_size=%d\n",mem_size);
    return float_buffer;
}

unsigned int* read_indices(cgltf_accessor* accessor)
{
    size_t available_numbers = index_count(accessor);
    //printf("available_numbers=%d\n",available_numbers);
    int mem_size = cgltf_accessor_unpack_indices(accessor, NULL, sizeof(int), available_numbers) * sizeof(int);
    unsigned int* index_buffer = (unsigned int*)malloc(mem_size);
    //cgltf_accessor_unpack_indices(accessor, index_buffer, sizeof(int), available_numbers);
    printf("indices_mem_size=%d\n",mem_size);
    return index_buffer;
}

int vertex_data_type(cgltf_primitive* primitive)
{
    int i;
    for(i = 0; i < primitive->attributes_count; i++)
    {
        if(primitive->attributes[i].type == cgltf_attribute_type_position)
        {
            return (primitive->attributes[i].data->type);
        }
    }
    return 0;
}

int data_type(cgltf_primitive* primitive, cgltf_attribute_type attribute_type)
{
    int i;
    for(i = 0; i < primitive->attributes_count; i++)
    {
        if(primitive->attributes[i].type == attribute_type)
        {
            return (primitive->attributes[i].data->type);
        }
    }
    return 0;
}

#define texcoord_data_type(primitive) data_type(primitive, cgltf_attribute_type_texcoord)

cgltf_accessor* get_position_accessor(cgltf_primitive* primitive)
{
    int i;
    for(i = 0; i < primitive->attributes_count; i++)
    {
        if(primitive->attributes[i].type == cgltf_attribute_type_position)
        {
            return (primitive->attributes[i].data);
        }
    }
    return NULL;
}

cgltf_accessor* get_accessor(cgltf_primitive* primitive, cgltf_attribute_type attribute_type)
{
    int i;
    for(i = 0; i < primitive->attributes_count; i++)
    {
        if(primitive->attributes[i].type == attribute_type)
        {
            return (primitive->attributes[i].data);
        }
    }
    return NULL;
}

#define get_texcoord_accessor(primitive) get_accessor(primitive, cgltf_attribute_type_texcoord)

cgltf_attribute* get_attribute(cgltf_primitive* primitive, cgltf_attribute_type type)
{
    int i;
    for(i = 0; i < primitive->attributes_count; i++)
    {
        if(primitive->attributes[i].type == type)
        {
            return (&primitive->attributes[i]);
        }
    }
    return NULL;
}

void print_vertex_data_type(cgltf_primitive* primitive)
{
    switch(vertex_data_type(primitive))
    {
        case cgltf_type_vec2:
            printf("vertex_data_type: vec2 \n");
            break;
        case cgltf_type_vec3:
            printf("vertex_data_type: vec3 \n");
            break;
        case cgltf_type_vec4:
            printf("vertex_data_type: vec4 \n");
            break;
        default:
            printf("vertex_data_type: is not a vertex data \n");
    }
}

void print_geometry(float* geometry, cgltf_primitive* primitive)
{
    typedef struct
    {
        float x,y,z;
    }vec3;
    vec3* buf_ptr = (vec3*)geometry;
    int vertex_count = get_position_accessor(primitive)->count;
    printf("\n ... printing geometry ... \n");
    printf("mesh vertex_count: %d \n",vertex_count);
    int i, num_vertex = 0, triangle = 1;
    for(i = 0; i < vertex_count; i++)
    {
        if(num_vertex == 0)
        {
            printf("triangle %d \n",triangle);
            triangle += 1;
        }
        printf("vertex %d: x=%f y=%f z=%f \n", i, buf_ptr[i].x, buf_ptr[i].y, buf_ptr[i].z);
        (num_vertex += 1) %= 3;
    }
     printf("\n");
}

void print_texcoord(float* text_coord, cgltf_primitive* primitive)
{
    typedef struct
    {
        float x,y;
    }vec2;
    vec2* buf_ptr = (vec2*)text_coord;
    int text_coord_count = get_texcoord_accessor(primitive)->count;
    printf("\n ... printing texture coordinates ... \n");
    printf("mesh texcoord_count: %d \n",text_coord_count);
    int i, num_vertex = 0, triangle = 1;
    for(i = 0; i < text_coord_count; i++)
    {
        if(num_vertex == 0)
        {
            printf("triangle %d \n",triangle);
            triangle += 1;
        }
        printf("texcoord %d: x=%f y=%f \n", i, buf_ptr[i].x, buf_ptr[i].y);
        (num_vertex += 1) %= 3;
    }
     printf("\n");
}

void print_indices(unsigned int* index_buffer, cgltf_primitive* primitive)
{

    int indices_count = primitive->indices->count;
    printf("\n ... printing indices ... \n");
    printf("mesh indices_count: %d \n",indices_count);
    int i, num_vertex = 0, triangle = 1;
    for(i = 0; i < indices_count; i++)
    {
        if(num_vertex == 0)
        {
            printf("triangle %d \n",triangle);
            triangle += 1;
        }
        printf("index %d: %d \n", i, index_buffer[i]);
        (num_vertex += 1) %= 3;
    }
     printf("\n");
}

float* read_primitive_geometry(cgltf_primitive* primitive)
{
    if(vertex_data_type(primitive) == cgltf_type_vec3)
    {
        return read_accessor(get_position_accessor(primitive));
    }
    else
    {
        printf("vertex data type is not vec3 \n");
    }
    return NULL;
}

float* read_primitive_texcoord(cgltf_primitive* primitive)
{
    if(texcoord_data_type(primitive) == cgltf_type_vec2)
    {
        return read_accessor(get_texcoord_accessor(primitive));
    }
    else
    {
        printf("vertex data type is not vec3 \n");
    }
    return NULL;
}

float* merge_geometry_and_textcoord(float* geometry, float* text_coord, cgltf_primitive* primitive)
{
    unsigned int geometry_size = float_buffer_size(get_position_accessor(primitive));
    unsigned int text_coord_size = float_buffer_size(get_texcoord_accessor(primitive));
    unsigned int offset = float_count(get_position_accessor(primitive));
    printf("merge_buffer_size=%d \n",geometry_size + text_coord_size);
    float* merge_buffer = (float*)malloc(geometry_size + text_coord_size);
    memcpy(merge_buffer, geometry, geometry_size);
    memcpy(&merge_buffer[offset], text_coord, text_coord_size);;
    return merge_buffer;
}

float* interleave_data(float* geometry, float* text_coord, cgltf_primitive* primitive)
{
    typedef struct
    {
        float x,y,z;
    }vec3;
    typedef struct
    {
        float x,y;
    }vec2;
    typedef struct
    {
        vec3 position;
        vec2 textcoord;
    }vertex_data;
    unsigned int geometry_size = float_buffer_size(get_position_accessor(primitive));
    unsigned int text_coord_size = float_buffer_size(get_texcoord_accessor(primitive));
    printf("interleaved_data_size=%d \n",geometry_size + text_coord_size);
    float* interleaved_data = (float*)malloc(geometry_size + text_coord_size);
    vec3* pos_data = (vec3*)geometry;
    vec2* textcoord_data = (vec2*)text_coord;
    vertex_data* data = (vertex_data*)interleaved_data;
    int vertex_count = get_position_accessor(primitive)->count;
    int i;
    for(i = 0; i < vertex_count; i++)
    {
        data[i].position = pos_data[i];
        data[i].textcoord = textcoord_data[i];
    }
    return interleaved_data;
}

float** read_mesh_geometry(cgltf_mesh* mesh)
{
    int primitives_count = mesh->primitives_count;
    float** float_buffers = (float**)malloc(primitives_count);
    int i;
    for(i = 0; i < primitives_count; i++)
    {
        float_buffers[i] = read_primitive_geometry(&mesh->primitives[i]);
    }
    return float_buffers;
}

void free_mesh_geometry(cgltf_mesh* mesh, float** float_buffers)
{
    int i;
    for(i = 0; i < mesh->primitives_count; i++)
    {
        free(float_buffers[i]); float_buffers[i] = NULL;
    }
    free(float_buffers); float_buffers = NULL;
}

size_t buffer_base64_size(char* base64)
{
    unsigned int length = strlen(base64) - 1;
    while(base64[length] == '=') length--;
    char *last_char_ptr = &base64[length];

    unsigned int buffer_bits = 0;

    for (cgltf_size i = 0; i < length; ++i)
	{
		while (buffer_bits < 8)
		{
			char ch = *base64++;
			if(base64 == last_char_ptr)
            {
                return i + 1;
            }

			int index =
				(unsigned)(ch - 'A') < 26 ? (ch - 'A') :
				(unsigned)(ch - 'a') < 26 ? (ch - 'a') + 26 :
				(unsigned)(ch - '0') < 10 ? (ch - '0') + 52 :
				ch == '+' ? 62 :
				ch == '/' ? 63 :
				-1;

			if (index < 0)
			{
				return 0;
			}

			buffer_bits += 6;
		}
		buffer_bits -= 8;
	}
	return 0;
}

void translate_geometry(float x, float y, float z, float *buffer, cgltf_primitive* primitive)
{
    typedef struct
    {
        float x,y,z;
    }vec3;
    vec3* buf_ptr = (vec3*)buffer;
    int vertex_count = get_position_accessor(primitive)->count;
    int i;
    for(i = 0; i < vertex_count; i++)
    {
        buf_ptr[i].x += x; buf_ptr[i].y += y; buf_ptr[i].z += z;
    }
}

void scale_down_geometry(float factor, float *buffer, cgltf_primitive* primitive)
{
    typedef struct
    {
        float x,y,z;
    }vec3;
    vec3* buf_ptr = (vec3*)buffer;
    int vertex_count = get_position_accessor(primitive)->count;
    int i;
    for(i = 0; i < vertex_count; i++)
    {
        buf_ptr[i].x /= factor; buf_ptr[i].y /= factor; //buf_ptr[i].z /= factor;
        buf_ptr[i].z = 0;
    }
}

void print_trs(cgltf_node* node)
{
    printf("\n ... printing TRS ... \n");
    if(node->has_translation)
    {
        float *trans = node->translation;
        printf("translation: x=%f y=%f z=%f \n", trans[0],  trans[1],  trans[2]);
    }
    else printf("node has no translation \n");

    if(node->has_rotation)
    {
        float *rot = node->rotation;
        printf("rotation: mag=%f x=%f y=%f z=%f \n", rot[0],  rot[1],  rot[2], rot[3]);
    }
    else printf("node has no rotation \n");

    if(node->has_scale)
    {
        float *scale = node->scale;
        printf("scale: x=%f y=%f z=%f \n", scale[0],  scale[1],  scale[2]);
    }
    else printf("node has no scale \n");
}

glm::mat4 interpolate_position(Animation_Data* anim_data, float animation_time);
glm::mat4 interpolate_rotation(Animation_Data* anim_data, float animation_time);
glm::mat4 interpolate_scaling(Animation_Data* anim_data, float animation_time);

Animation_Data* read_animation_data(cgltf_animation_channel* channel)
{
    Animation_Data* data = (Animation_Data*)malloc(sizeof(Animation_Data));
    data->type = channel->target_path;
    data->count = channel->sampler->input->count;
    data->time = read_accessor(channel->sampler->input);
    data->trs = read_accessor(channel->sampler->output);
    switch(data->type)
    {
        case cgltf_animation_path_type_translation:
            data->interpolate_animation = interpolate_position;
            break;
        case cgltf_animation_path_type_rotation:
            data->interpolate_animation = interpolate_rotation;
            break;
        case cgltf_animation_path_type_scale:
            data->interpolate_animation = interpolate_scaling;
            break;
    }
    return data;
}

void free_animation_data(Animation_Data* data)
{
    free(data->time); data->time = NULL;
    free(data->trs); data->trs = NULL;
    free(data); data = NULL;
}

void print_animation_data(cgltf_animation_channel* channel)
{
    Animation_Data* data;
    char* animation_type[5] = {"invalid", "translation", "rotation", "scale", "weights"};
    char* interpolation_type[3] = {"linear", "step", "cubic_spline"};
    printf("\n ... printing animation data ... \n");
    printf("animation type: %s \n",animation_type[channel->target_path]);
    printf("interpolation type: %s \n",interpolation_type[channel->sampler->interpolation]);
    printf("reading animation times/trs ... \n");
    data = read_animation_data(channel);
    printf("\n ... times ... \n");
    int i, j, elements_num, count = channel->sampler->input->count;
    for(i = 0; i < count ; i++)
    {
        printf("time%d= %f \n",i,data->time[i]);
    }
    printf("\n ... trs ... \n");
    if(data->type == cgltf_animation_path_type_weights)
    {
        elements_num = channel->sampler->output->count / channel->sampler->input->count;
        count = channel->sampler->output->count;
    }
    else
    {
        elements_num = data->type == 2 ? 4 : 3;
        count = channel->sampler->output->count * elements_num;
    }
    for(i = 0; i < count ; i += elements_num)
    {
        printf("trs%d= (%f", i/elements_num, data->trs[i]);
        for(j = 1; j < elements_num; j++)
        {
            printf(", %f", data->trs[i+j]);
        }
        printf(") \n");
    }
    free_animation_data(data);
}

glm::mat4 quat_to_mat(const glm::quat & quat_value)
{
    glm::quat quat_norm = glm::normalize(quat_value);
    return glm::toMat4(quat_norm);
}

/*glm::quat get_glm_quat(float* quat)
{
    return glm::quat(quat[3], quat[0], quat[1], quat[2]);
}*/

#define get_glm_quat(vec) glm::quat((vec)[3], (vec)[0], (vec)[1], (vec)[2])

void print_matrix(const glm::mat4 & matrix)
{
    int i, j;
    for(i = 0; i < 4; ++i)
    {
        for(j = 0; j < 4;++j)
        {
            printf("%f ", matrix[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void print_quat(const glm::quat & q)
{

     printf("quat: x=%f y=%f z=%f w=%f \n", q.x, q.y, q.z, q.w);
}

/* Gets the current index on Key Frame Rotations to interpolate to based on the
 current animation time*/
int get_animation_frame_index(float animation_current_time, float* animation_times, int animation_times_count)
{
    for (int index = 0; index < animation_times_count - 1; ++index)
    {
        if (animation_current_time < animation_times[index + 1])
            return index;
    }
    assert(0);
}

/* Gets normalized value for Lerp & Slerp*/
float get_scale_factor(float last_time_stamp, float next_time_stamp, float animation_time)
{
    float scale_factor = 0.0f;
    float midway_length = animation_time - last_time_stamp;
    float frames_diff = next_time_stamp - last_time_stamp;
    scale_factor = midway_length / frames_diff;
    return scale_factor;
}

glm::mat4 interpolate_position(Animation_Data* anim_data, float animation_time)
{
    int index_1 = get_animation_frame_index(animation_time, anim_data->time, anim_data->count);
    int index_2 = index_1 + 1;
    float scale_factor = get_scale_factor(anim_data->time[index_1],
        anim_data->time[index_2], animation_time);
    glm::vec3* positions = (glm::vec3*)anim_data->trs;
    glm::vec3 final_position = glm::mix(positions[index_1], positions[index_2], scale_factor);
    return glm::translate(glm::mat4(1.0f), final_position);
}

glm::mat4 interpolate_rotation(Animation_Data* anim_data, float animation_time)
{
    int index_1 = get_animation_frame_index(animation_time, anim_data->time, anim_data->count);
    int index_2 = index_1 + 1;
    float scale_factor = get_scale_factor(anim_data->time[index_1],
        anim_data->time[index_2], animation_time);
    glm::quat quat_1 = get_glm_quat(anim_data->trs + (index_1 << 2));
    glm::quat quat_2 = get_glm_quat(anim_data->trs + (index_2 << 2));
    glm::quat final_rotation = glm::slerp(quat_1, quat_2, scale_factor);
    final_rotation = glm::normalize(final_rotation);
    return glm::toMat4(final_rotation);
}

glm::mat4 interpolate_scaling(Animation_Data* anim_data, float animation_time)
{
    int index_1 = get_animation_frame_index(animation_time, anim_data->time, anim_data->count);
    int index_2 = index_1 + 1;
    float scale_factor = get_scale_factor(anim_data->time[index_1],
        anim_data->time[index_2], animation_time);
    glm::vec3* scales = (glm::vec3*)anim_data->trs;
    glm::vec3 final_scale = glm::mix(scales[index_1], scales[index_2], scale_factor);
    return glm::scale(glm::mat4(1.0f), final_scale);
}

glm::mat4 update_animation(Animation_Data* anim_data, Animation_Timer* anim_timer)
{
    //anim_currrent_time += anim_ticks_per_second * delta_time;
    //anim_currrent_time += anim_ticks_per_second;
    anim_timer->currrent_time += (anim_timer->delta_time / 1000);
    anim_timer->currrent_time = fmod(anim_timer->currrent_time, anim_timer->duration);
    return interpolate_rotation(anim_data, anim_timer->currrent_time);
}

void print_skin_data(cgltf_skin* skin)
{
    printf("\n ... printing skin data ... \n");
    printf("joints_count: %d \n", skin->joints_count);
    printf("reading inverse bind matrices ... \n");
    float* float_array_matrices = read_accessor(skin->inverse_bind_matrices);
    glm::mat4 matrices[skin->joints_count];
    printf("sizeof: glm::mat4= %d matrices= %d \n", sizeof(glm::mat4), sizeof(matrices));
    for(int i = 0; i < skin->joints_count; i++)
    {
        matrices[i] = glm::make_mat4(float_array_matrices + i * 16);
        printf("matrix %d : \n", i);
        print_matrix(matrices[i]);
    }
    free(float_array_matrices);
}

void print_bone_data(cgltf_primitive* primitive)
{
    typedef struct
    {
        float x, y, z, w;
    }vec4;
    printf("\n ... printing bone data ... \n");
    printf("printing vertices weights ... \n");
    printf("attributes_count: %d\n", primitive->attributes_count);
    cgltf_accessor* accessor = get_accessor(primitive, cgltf_attribute_type_weights);
    printf("weights_count: %d\n", accessor->count);
    float* weights_buffer = read_accessor(accessor);
    vec4* weights = (vec4*)weights_buffer;
    for(int i = 0; i < accessor->count; i++, weights++)
    {
        printf("v%d w: %f %f %f %f \n", i, weights->x, weights->y, weights->z, weights->w);
    }
    free(weights_buffer);
    printf("printing joints indices ... \n");
    accessor = get_accessor(primitive, cgltf_attribute_type_joints);
    printf("stride: %d \n",accessor->stride);
    float* indices_buffer = read_accessor(accessor);
    size_t count = index_count(accessor);
    for (int i = 0; i < count; i+=4)
    {
       printf("v%d j: %d %d %d %d \n", i/4, (int)(indices_buffer[i]),  (int)(indices_buffer[i+1]),
              (int)(indices_buffer[i+2]), (int)(indices_buffer[i+3]));
    }
    free(indices_buffer);
}

void get_trs_transform(cgltf_node* node, TRS_Transform *trs)
{
    float *vec;
    trs->trans = trs->rot = trs->scale = glm::mat4(1.0f);

    if(node->has_translation)
    {
        vec = node->translation;
        trs->trans =  glm::translate(trs->trans, glm::vec3(vec[0], vec[1], vec[2]));
    }

    if(node->has_rotation)
    {
        vec = node->rotation;
        glm::quat quat = glm::quat(vec[3], vec[0], vec[1], vec[2]);
        quat = glm::normalize(quat);
        trs->rot = glm::toMat4(quat);
    }

    if(node->has_scale)
    {
        vec = node->scale;
        trs->scale =  glm::scale(trs->scale, glm::vec3(vec[0], vec[1], vec[2]));
    }
}

glm::mat4* get_inverse_bind_matrices(cgltf_skin* skin)
{
    float* float_array_matrices = read_accessor(skin->inverse_bind_matrices);
    glm::mat4* matrices = new glm::mat4[skin->joints_count];
    for(int i = 0; i < skin->joints_count; i++)
    {
        matrices[i] = glm::make_mat4(float_array_matrices + i * 16);
    }
    free(float_array_matrices);
    return matrices;
}

Bone_Data* read_bone_data(cgltf_primitive* primitive)
{
    Bone_Data* data = (Bone_Data*)malloc(sizeof(Bone_Data));
    cgltf_accessor* accessor = get_accessor(primitive, cgltf_attribute_type_weights);
    data->weights = (Vec4*)read_accessor(accessor);
    data->weights_count = accessor->count;
    data->weights_size = accessor->count * sizeof(Vec4);
    accessor = get_accessor(primitive, cgltf_attribute_type_joints);
    data->joints = (Vec4*)read_accessor(accessor);
    data->joints_count = accessor->count;
    data->joints_size = accessor->count * sizeof(Vec4);
    return data;
}

void free_bone_data(Bone_Data* data)
{
    free(data->weights); data->weights = NULL;
    free(data->joints); data->joints = NULL;
    free(data); data = NULL;
}

void print_attribute_geometry(float* geometry, cgltf_attribute* attribute)
{
    typedef struct
    {
        float x,y,z;
    }vec3;
    vec3* buf_ptr = (vec3*)geometry;
    int vertex_count = attribute->data->count;
    printf("... printing attribute geometry ... \n");
    printf("mesh vertex_count: %d \n",vertex_count);
    int i, num_vertex = 0, triangle = 1;
    for(i = 0; i < vertex_count; i++)
    {
        if(num_vertex == 0)
        {
            printf("triangle %d \n",triangle);
            triangle += 1;
        }
        printf("vertex %d: x=%f y=%f z=%f \n", i, buf_ptr[i].x, buf_ptr[i].y, buf_ptr[i].z);
        (num_vertex += 1) %= 3;
    }
}

void print_morph_target(cgltf_primitive* primitive)
{
    printf("\n ... printing morph target ... \n");
    printf("targets_count: %d\n", primitive->targets_count);
    cgltf_morph_target* targets = primitive->targets;
    cgltf_attribute* attribute;
    float* vertex_array;
    for(int i = 0; i < primitive->targets_count; i++)
    {
        printf("target %d attributes_count: %d \n", i, targets[i].attributes_count);
        printf("printing target %d attributes ... \n",i);
        for(int j = 0; j < targets[i].attributes_count; j++)
        {
            attribute = &targets[i].attributes[j];
            printf("attribute %d name: %s \n", j, attribute->name);
            printf("attribute %d index: %d \n", j, attribute->index);
            printf("attribute %d type: %d \n", j, attribute->type);
            if(attribute->type == cgltf_attribute_type_position)
            {
                printf("reading attribute %d geometry \n", j);
                vertex_array = read_accessor(attribute->data);
                print_attribute_geometry(vertex_array, attribute);
                free(vertex_array);
            }
        }
        printf("\n");
    }
}

Morph_Target_Data* read_morph_target(cgltf_primitive* primitive)
{
    Morph_Target_Data* data = (Morph_Target_Data*)malloc(sizeof(Morph_Target_Data));
    data->meshes = (Mesh_Data*)malloc(sizeof(Mesh_Data) * primitive->targets_count);
    data->meshes_count = primitive->targets_count;
    cgltf_morph_target* targets = primitive->targets;
    cgltf_attribute* attribute; float* vertex_array;
    for(int i = 0; i < primitive->targets_count; i++)
    {
        for(int j = 0; j < targets[i].attributes_count; j++)
        {
            attribute = &targets[i].attributes[j];
            if(attribute->type == cgltf_attribute_type_position)
            {
                vertex_array = read_accessor(attribute->data);
                data->meshes[i].vertices = (Vec3*)vertex_array;
                data->meshes[i].vertices_count = attribute->data->count;
                data->meshes[i].vertices_size = attribute->data->count * sizeof(Vec3);
            }
        }
    }
    return data;
}

void free_morph_target(Morph_Target_Data* data)
{
    for(int i = 0; i < data->meshes_count; i++)
    {
        free(data->meshes[i].vertices); data->meshes[i].vertices = NULL;
    }
    data->meshes = NULL;
    free(data); data = NULL;
}

glm::vec2 interpolate_weight(Animation_Data* anim_data, float animation_time)
{
    int index_1 = get_animation_frame_index(animation_time, anim_data->time, anim_data->count);
    int index_2 = index_1 + 1;
    float scale_factor = get_scale_factor(anim_data->time[index_1],
        anim_data->time[index_2], animation_time);
    glm::vec2* weights = (glm::vec2*)anim_data->trs;
    glm::vec2 final_weight = glm::mix(weights[index_1], weights[index_2], scale_factor);
    return final_weight;
}

glm::vec2 update_morph_animation(Animation_Data* anim_data, Animation_Timer* anim_timer)
{
    anim_timer->currrent_time += (anim_timer->delta_time / 1000);
    anim_timer->currrent_time = fmod(anim_timer->currrent_time, anim_timer->duration);
    return interpolate_weight(anim_data, anim_timer->currrent_time);
}

void setup_mesh(Mesh_Data* mesh)
{
    // create buffers/arrays
    glGenVertexArrays(1, &mesh->VAO);
    glGenBuffers(2, mesh->VBO);
    glGenBuffers(1, &mesh->EBO);

    // load data into buffers
    glBindVertexArray(mesh->VAO);

    // position attribute
    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, mesh->vertices_size, mesh->vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // texture coordinate attribute
    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, mesh->texcoord_size, mesh->texcoord, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

Mesh_Data* load_mesh(cgltf_mesh* mesh)
{
    Mesh_Data* data = (Mesh_Data*)malloc(sizeof(Mesh_Data));
    cgltf_accessor* accessor = get_position_accessor(&mesh->primitives[0]);
    data->vertices = (Vec3*)read_accessor(accessor);;
    data->vertices_count = accessor->count;
    data->vertices_size = accessor->count * sizeof(Vec3);
    accessor = get_texcoord_accessor(&mesh->primitives[0]);
    data->texcoord = (Vec2*)read_accessor(accessor);;
    data->texcoord_count = accessor->count;
    data->texcoord_size = accessor->count * sizeof(Vec2);
    setup_mesh(data);
    return data;
}

void free_mesh(Mesh_Data* mesh)
{
    glDeleteVertexArrays(1, &mesh->VAO);
    glDeleteBuffers(2, mesh->VBO);
    glDeleteBuffers(1, &mesh->EBO);
    free(mesh->vertices); mesh->vertices = NULL;
    free(mesh->texcoord); mesh->texcoord = NULL;
    free(mesh); mesh = NULL;
}

void draw_mesh(Mesh_Data* mesh)
{
    // bind textures on corresponding texture units
    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, texture);
    // draw mesh
    glBindVertexArray(mesh->VAO);
    glDrawArrays(GL_TRIANGLES, 0, mesh->vertices_count);
}


unsigned int load_texture_from_memory(cgltf_texture* gltf_texture, cgltf_options* options)
{
    unsigned char *image_buffer;
	const char* comma = strchr(gltf_texture->image->uri, ',');
	unsigned int base64_size = buffer_base64_size((char*)comma + 1);
	void (*memory_free)(void*, void*) = options->memory.free_func ? options->memory.free_func : &cgltf_default_free;
	cgltf_load_buffer_base64(options, base64_size, comma + 1, (void**)&image_buffer);

    int texture_wrap_s = gltf_texture->sampler->wrap_s;
    int texture_wrap_t = gltf_texture->sampler->wrap_t;
    int texture_min_filter = gltf_texture->sampler->min_filter;
    int texture_mag_filter = gltf_texture->sampler->mag_filter;

    // load and create a texture
    // -------------------------
    unsigned int texture;
    glGenTextures(1, &texture);
    // if your texture code doesn't work or shows up as completely black
    // that's because On some drivers it is required to assign a texture unit to each sampler uniform
    // GL_TEXTURE0 is always by default activated
    //glActiveTexture(GL_TEXTURE0); // activate the texture unit first before binding texture
    glBindTexture(GL_TEXTURE_2D, texture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture_wrap_s);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture_wrap_t);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture_min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture_mag_filter);
    // load image, create texture and generate mipmaps
    int width, height, nrChannels;
    //stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    unsigned char *data = stbi_load_from_memory(image_buffer, base64_size, &width, &height, &nrChannels, 0);
    memory_free(options->memory.user_data, image_buffer);
    if (data)
    {
        // note that png has transparency and thus an alpha channel, so make sure to tell OpenGL the data type is of GL_RGBA
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        printf("Failed to load texture \n");
    }
    stbi_image_free(data);

    return texture;
}

Model_Animation* load_model_animation(cgltf_animation* gltf_anim, cgltf_node* gltf_nodes, Animation_Node** anim_nodes)
{
    unsigned int anim_data_count = gltf_anim->channels_count;
    Model_Animation* model_anim = (Model_Animation*)malloc(sizeof(Model_Animation));
    model_anim->anim_data = (Animation_Data**)malloc(sizeof(Animation_Data*) * anim_data_count);
    model_anim->anim_data_count = anim_data_count;
    unsigned int index;
    for(unsigned int i = 0; i < anim_data_count; i++)
    {
        model_anim->anim_data[i] = read_animation_data(&gltf_anim->channels[i]);
        index = gltf_anim->channels[i].target_node - gltf_nodes;
        model_anim->anim_data[i]->target_node = anim_nodes[index];
    }
    Animation_Data* anim_data = model_anim->anim_data[0];
    model_anim->duration = anim_data->time[anim_data->count-1];
    return model_anim;
}

void free_model_animation(Model_Animation* model_anim)
{
    for(unsigned int i = 0; i < model_anim->anim_data_count; i++)
    {
        free_animation_data(model_anim->anim_data[i]); model_anim->anim_data[i] = NULL;
    }
    free(model_anim->anim_data);  model_anim->anim_data = NULL;
    free(model_anim);  model_anim = NULL;
}

void print_animation_data_2(Animation_Data* data)
{
    char* animation_type[5] = {"invalid", "translation", "rotation", "scale", "weights"};
    //char* interpolation_type[3] = {"linear", "step", "cubic_spline"};
    printf("\n ... printing animation data ... \n");
    printf("animation type: %s \n",animation_type[data->type]);
    //printf("target node: %d \n",data->target_node);
    //printf("interpolation type: %s \n",interpolation_type[channel->sampler->interpolation]);
    //printf("reading animation times/trs ... \n");
    //data = read_animation_data(channel);
    printf("\n ... times ... \n");
    int i, j, elements_num, count = data->count;
    for(i = 0; i < count ; i++)
    {
        printf("time%d= %f \n",i,data->time[i]);
    }
    printf("\n ... trs ... \n");
    elements_num = data->type == 2 ? 4 : 3;
    count = data->count * elements_num;
    for(i = 0; i < count ; i += elements_num)
    {
        printf("trs%d= (%f", i/elements_num, data->trs[i]);
        for(j = 1; j < elements_num; j++)
        {
            printf(", %f", data->trs[i+j]);
        }
        printf(") \n");
    }
}

void get_node_children_and_parent(Animation_Node* anim_node, cgltf_node* gltf_node, cgltf_data* gltf_data, Model_Data* model)
{
    unsigned int index;
    if(gltf_node->parent != NULL)
    {
        index = gltf_node->parent - gltf_data->nodes;
        anim_node->parent = model->anim_nodes[index];
    }
    else anim_node->parent = NULL;
    anim_node->children_count = gltf_node->children_count;
    for(int i = 0; i < gltf_node->children_count; i ++)
    {
        index = gltf_node->children[i] - gltf_data->nodes;
        anim_node->children[i] = model->anim_nodes[index];
    }
}

Animation_Node* load_animation_node(cgltf_node* gltf_node, cgltf_data* gltf_data, Model_Data* model)
{
    Animation_Node* anim_node = (Animation_Node*)malloc(sizeof(Animation_Node));
    unsigned int index;
    if(gltf_node->mesh != NULL)
    {
        index = gltf_node->mesh - gltf_data->meshes;
        anim_node->mesh = model->meshes[index];
    }
    else anim_node->mesh = NULL;
    anim_node->children = (Animation_Node**)malloc(sizeof(Animation_Node*) * gltf_node->children_count);
    anim_node->trs.trans = anim_node->trs.rot = anim_node->trs.scale = glm::mat4(1.0f);
    anim_node->local_transform = anim_node->global_transform = glm::mat4(1.0f);
    anim_node->trans_anim = anim_node->rot_anim = anim_node->scale_anim = NULL;
    return anim_node;
}

void free_animation_node(Animation_Node* anim_node)
{
    free(anim_node->children);  anim_node->children = NULL;
    free(anim_node);  anim_node = NULL;
}

void get_animation_node_trs_transform(Animation_Node* node, Animation_Data* anim_data, int index)
{
    float *vec = anim_data->trs;

    switch(anim_data->type)
    {
        case cgltf_animation_path_type_translation:
            vec += 3*index;
            node->trs.trans = glm::mat4(1.0f);
            node->trs.trans =  glm::translate(node->trs.trans, glm::vec3(vec[0], vec[1], vec[2]));
            break;
        case cgltf_animation_path_type_rotation:
        {
            vec += 4*index;
            node->trs.rot = glm::mat4(1.0f);
            glm::quat quat = glm::quat(vec[3], vec[0], vec[1], vec[2]);
            quat = glm::normalize(quat);
            node->trs.rot = glm::toMat4(quat);
            break;
        }
        case cgltf_animation_path_type_scale:
            vec += 3*index;
            node->trs.scale = glm::mat4(1.0f);
            node->trs.scale = glm::scale(node->trs.scale, glm::vec3(vec[0], vec[1], vec[2]));
            break;
    }
}

void calculate_animation_nodes_transform(Animation_Node* root_node)
{
    TRS_Transform* trs = &root_node->trs;
    root_node->local_transform = trs->trans * trs->rot * trs->scale;
    if(root_node->parent != NULL)
        root_node->global_transform = root_node->parent->global_transform * root_node->local_transform;
    else
        root_node->global_transform = root_node->local_transform;
    if(root_node->mesh != NULL)
        root_node->mesh->bone_matrix = root_node->global_transform;
    for(int i = 0; i < root_node->children_count; i++)
    {
        calculate_animation_nodes_transform(root_node->children[i]);
    }
}

void load_animation_frame(Model_Data* model, Model_Animation* animation, int frame_index)
{
    for(int i = 0; i < animation->anim_data_count; i++)
    {
        get_animation_node_trs_transform(animation->anim_data[i]->target_node, animation->anim_data[i], frame_index);
    }
    for(int i = 0; i < model->root_nodes_count; i++)
    {
        calculate_animation_nodes_transform(model->root_nodes[i]);
    }
}

Animation_Node** get_root_nodes(Model_Data* model, cgltf_data* gltf_data)
{
    unsigned int index;
    model->root_nodes_count = gltf_data->scene->nodes_count;
    Animation_Node** root_nodes = (Animation_Node**)malloc(sizeof(Animation_Node*) * gltf_data->scene->nodes_count);
    for(int i = 0; i < gltf_data->scene->nodes_count; i++)
    {
        index = gltf_data->scene->nodes[i] - gltf_data->nodes;
        root_nodes[i] = model->anim_nodes[index];
    }
    return root_nodes;
}

Model_Data* load_model(cgltf_data* gltf_data, cgltf_options* options)
{
    unsigned int meshes_count = gltf_data->meshes_count;
    Model_Data* model = (Model_Data*)malloc(sizeof(Model_Data));
    model->meshes = (Mesh_Data**)malloc(sizeof(Mesh_Data*) * meshes_count);
    model->meshes_count = meshes_count;
    for(unsigned int i = 0; i < meshes_count; i++)
    {
        model->meshes[i] = load_mesh(&gltf_data->meshes[i]);
    }
    model->texture = load_texture_from_memory(&gltf_data->textures[0], options);
    model->anim_nodes_count = gltf_data->nodes_count;
    model->anim_nodes = (Animation_Node**)malloc(sizeof(Animation_Node*) * gltf_data->nodes_count);
    for(unsigned int i = 0; i < gltf_data->nodes_count; i++)
    {
        model->anim_nodes[i] = load_animation_node(&gltf_data->nodes[i], gltf_data, model);
    }
    for(unsigned int i = 0; i < gltf_data->nodes_count; i++)
    {
        get_node_children_and_parent(model->anim_nodes[i], &gltf_data->nodes[i], gltf_data, model);
    }
    model->root_nodes_count = gltf_data->scene->nodes_count;
    model->root_nodes = get_root_nodes(model, gltf_data);
    model->animations_count = gltf_data->animations_count;
    model->animations = (Model_Animation**)malloc(sizeof(Model_Animation*) * gltf_data->animations_count);
    for(unsigned int i = 0; i < gltf_data->animations_count; i++)
    {
        model->animations[i] = load_model_animation(&gltf_data->animations[i], gltf_data->nodes, model->anim_nodes);
    }
    model->curren_animation = model->animations[0];
    model->animation_time = 0.0;
    return model;
}

void free_model(Model_Data* model)
{
    for(unsigned int i = 0; i < model->meshes_count; i++)
    {
        free_mesh(model->meshes[i]); model->meshes[i] = NULL;
    }
    free(model->meshes);  model->meshes = NULL;
    glDeleteTextures(1, &model->texture);
    for(unsigned int i = 0; i < model->anim_nodes_count; i++)
    {
        free_animation_node(model->anim_nodes[i]); model->anim_nodes[i] = NULL;
    }
    free(model->anim_nodes);  model->anim_nodes = NULL;
    free(model->root_nodes);  model->root_nodes = NULL;
    for(unsigned int i = 0; i < model->animations_count; i++)
    {
        free_model_animation(model->animations[i]); model->animations[i] = NULL;
    }
    free(model->animations);  model->animations = NULL;
    free(model);  model = NULL;
}

Model_Data* load_gltf_model(char* model_file)
{
    cgltf_options options;
	memset(&options, 0, sizeof(cgltf_options));
	cgltf_data* gltf_data = NULL;
	cgltf_result result = cgltf_parse_file(&options, model_file, &gltf_data);

    if (result == cgltf_result_success)
		result = cgltf_load_buffers(&options, gltf_data, model_file);
    else
        printf("could not parse gltf file: %s \n", model_file);

	if (result == cgltf_result_success)
		result = cgltf_validate(gltf_data);
    else
         printf("could not load buffers ! \n");

    Model_Data* model = NULL;

    if(result == cgltf_result_success)
        model = load_model(gltf_data, &options);

    cgltf_free(gltf_data);

    return model;
}

void draw_model(Model_Data* model, unsigned int shader_id)
{
    Mesh_Data* mesh;
    unsigned int bone_matrix_location = glGetUniformLocation(shader_id, "bone_matrix");
    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, model->texture);
    // render model
    for (unsigned int i = 0; i < model->meshes_count; i++)
    {
        mesh = model->meshes[i];
        glUniformMatrix4fv(bone_matrix_location, 1, GL_FALSE, &mesh->bone_matrix[0][0]);
        glBindVertexArray(mesh->VAO);
        glDrawArrays(GL_TRIANGLES, 0, mesh->vertices_count);
    }
    // always good practice to set everything back to defaults once configured.
    //glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(0);
}

void interpolate_node_animation(Animation_Node* node, Animation_Data* anim_data, float currrent_time)
{
    switch(anim_data->type)
    {
        case cgltf_animation_path_type_translation:
            node->trs.trans =  interpolate_position(anim_data, currrent_time);
            break;
        case cgltf_animation_path_type_rotation:
            node->trs.rot = interpolate_rotation(anim_data, currrent_time);
            break;
        case cgltf_animation_path_type_scale:
            node->trs.scale = interpolate_scaling(anim_data, currrent_time);
            break;
    }
}

void step_interpolate_node_animation(Animation_Node* node, Animation_Data* anim_data, float currrent_time)
{
    switch(anim_data->type)
    {
        case cgltf_animation_path_type_translation:
        {
            int index = get_animation_frame_index(currrent_time, anim_data->time, anim_data->count);
            glm::vec3* positions = (glm::vec3*)anim_data->trs;
            node->trs.trans =  glm::translate(glm::mat4(1.0f), positions[index]);
            break;
        }
        case cgltf_animation_path_type_rotation:
        {
            int index = get_animation_frame_index(currrent_time, anim_data->time, anim_data->count);
            glm::quat quat_rotation = get_glm_quat(anim_data->trs+index*4);
            quat_rotation = glm::normalize(quat_rotation);
            node->trs.rot = glm::toMat4(quat_rotation);;
            break;
        }
        case cgltf_animation_path_type_scale:
        {
            int index = get_animation_frame_index(currrent_time, anim_data->time, anim_data->count);
            glm::vec3* scales = (glm::vec3*)anim_data->trs;
            node->trs.scale = glm::scale(glm::mat4(1.0f), scales[index]);
            break;
        }
    }
}

void update_animation_frame(Model_Data* model, Model_Animation* animation, float currrent_time)
{
    for(int i = 0; i < animation->anim_data_count; i++)
    {
        interpolate_node_animation(animation->anim_data[i]->target_node, animation->anim_data[i], currrent_time);
    }
    for(int i = 0; i < model->root_nodes_count; i++)
    {
        calculate_animation_nodes_transform(model->root_nodes[i]);
    }
}

void update_animation_frame_2(Model_Data* model, float currrent_time)
{
    Animation_Node* anim_node;
    TRS_Transform* trs;
    for(int i = 0; i < model->anim_nodes_count; i++)
    {
        anim_node = model->anim_nodes[i];
        trs = &anim_node->trs;
        if(anim_node->trans_anim)
            trs->trans =  anim_node->trans_anim->interpolate_animation(anim_node->trans_anim, currrent_time);
        if(anim_node->rot_anim)
            trs->rot =  anim_node->rot_anim->interpolate_animation(anim_node->rot_anim, currrent_time);
        if(anim_node->scale_anim)
            trs->scale =  anim_node->scale_anim->interpolate_animation(anim_node->scale_anim, currrent_time);
        anim_node->local_transform = trs->trans * trs->rot * trs->scale;
        if(anim_node->parent != NULL)
            anim_node->global_transform = anim_node->parent->global_transform * anim_node->local_transform;
        else
            anim_node->global_transform = anim_node->local_transform;
        if(anim_node->mesh != NULL)
            anim_node->mesh->bone_matrix = anim_node->global_transform;
    }
}

void update_skeletal_animation(Model_Data* model, float delta_time)
{
    model->animation_time += (delta_time / 1000);
    model->animation_time = fmod(model->animation_time, model->curren_animation->duration);
    update_animation_frame(model, model->curren_animation, model->animation_time);
    //update_animation_frame_2(model, model->animation_time);
}

void load_animation_data(Model_Animation* animation)
{
    Animation_Data* anim_data;
    for(int i = 0; i < animation->anim_data_count; i++)
    {
        anim_data = animation->anim_data[i];
        switch(anim_data->type)
        {
            case cgltf_animation_path_type_translation:
                anim_data->target_node->trans_anim =  anim_data;
                break;
            case cgltf_animation_path_type_rotation:
                anim_data->target_node->rot_anim =  anim_data;
                break;
            case cgltf_animation_path_type_scale:
                anim_data->target_node->scale_anim =  anim_data;
                break;
        }
    }
}

void change_model_animation(Model_Data* model, int animation_index)
{
    if(animation_index >= 0 && animation_index <  model->animations_count)
    {
        model->curren_animation = model->animations[animation_index];
        //load_animation_data(model->animations[animation_index]);
    }
    model->animation_time = 0.0;
}

#endif // GLTF_LOADERL_H
